"""Minimal icon/NACP NRO packager, following switchbrew/switch-tools elf2nro.c.
No RomFS support. The application itself is compiled with devkitA64, not Python.
"""
from pathlib import Path
import struct, sys

def package(elf_path, icon_path, output):
    elf=Path(elf_path).read_bytes()
    assert elf[:6] == b'\x7fELF\x02\x01'
    assert struct.unpack_from('<H',elf,18)[0] == 183
    phoff,shoff=struct.unpack_from('<QQ',elf,32)
    phsize,phnum,shsize,shnum=struct.unpack_from('<HHHH',elf,54)
    loads=[]
    for i in range(phnum):
        typ,flags,off,va,pa,fs,ms,align=struct.unpack_from('<IIQQQQQQ',elf,phoff+i*phsize)
        if typ==1: loads.append((off,va,fs,ms))
    assert len(loads)==3
    up=lambda n:(n+4095)&~4095
    total=sum(up(x[2]) for x in loads)
    out=bytearray(total)
    for off,va,fs,ms in loads:
        assert off+fs<=len(elf) and va+up(fs)<=total
        out[va:va+fs]=elf[off:off+fs]
    hdr=bytearray(0x70)
    struct.pack_into('<4sIII',hdr,0,b'NRO0',0,total,0)
    for i,(off,va,fs,ms) in enumerate(loads): struct.pack_into('<II',hdr,16+8*i,va,up(fs))
    struct.pack_into('<I',hdr,40,up(max(0,loads[2][3]-up(loads[2][2]))))
    for i in range(shnum):
        name,typ,flags,va,off,size=struct.unpack_from('<IIQQQQ',elf,shoff+i*shsize)
        if typ==7:
            namesz,descsz,ntype=struct.unpack_from('<III',elf,off)
            if ntype==3 and elf[off+12:off+16]==b'GNU\0':
                desc=elf[off+16:off+16+min(descsz,32)]
                hdr[48:48+len(desc)]=desc
    out[16:128]=hdr
    nacp=bytearray(0x4000)
    for i in range(12):
        title=b'SwitchTalk Overlay' if Path(output).suffix=='.ovl' else b'SwitchTalk Worker'; author=b'Monster Review'
        nacp[i*0x300:i*0x300+len(title)]=title
        nacp[i*0x300+0x200:i*0x300+0x200+len(author)]=author
    nacp[0x3060:0x3065]=b'1.0.1'
    for off,val in [(0x3024,0x100),(0x302c,0xbff),(0x3034,0x10000),(0x3080,0x3e00000),(0x3088,0x180000),(0x30f0,0x102)]:
        struct.pack_into('<I',nacp,off,val)
    nacp[0x3040:0x3060]=bytes([12,255,255,10,255,12,12,12,12,12,13,13]+[255]*20)
    icon=Path(icon_path).read_bytes()
    out+=struct.pack('<4sIQQQQQQ',b'ASET',0,56,len(icon),56+len(icon),len(nacp),0,0)
    out+=icon+nacp
    Path(output).write_bytes(out)
    print(f'Packaged {output}: {len(out)} bytes')

if __name__=='__main__': package(*sys.argv[1:])
