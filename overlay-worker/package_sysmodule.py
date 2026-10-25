"""Package a linked AArch64 ELF as uncompressed NSO and PFS0 ExeFS.
NSO layout follows switchbrew switch-tools elf2nso.c (COPYING.switch-tools).
Usage: python package_sysmodule.py background.elf main.npdm output-directory
"""
from pathlib import Path
import struct,hashlib,sys

def pack(elf_path,npdm_path,directory):
 elf=Path(elf_path).read_bytes();npdm=Path(npdm_path).read_bytes();directory=Path(directory);directory.mkdir(parents=True,exist_ok=True)
 assert elf[:4]==b'\x7fELF' and struct.unpack_from('<H',elf,18)[0]==183
 phoff=struct.unpack_from('<Q',elf,32)[0];phsize,phnum=struct.unpack_from('<HH',elf,54)
 loads=[]
 for i in range(phnum):
  typ,flags,off,va,pa,fs,ms,al=struct.unpack_from('<IIQQQQQQ',elf,phoff+i*phsize)
  if typ==1:loads.append((off,va,fs,ms))
 assert len(loads)==3 and loads[0][1]==0
 h=bytearray(256);h[:4]=b'NSO0';struct.pack_into('<I',h,12,0x38) # verify three hashes; no compression
 sections=[];file_off=256
 for i,(off,va,fs,ms) in enumerate(loads):
  assert off+fs<=len(elf) and fs<=ms and va%4096==0
  data=elf[off:off+fs];sections.append(data)
  struct.pack_into('<IIII',h,16+i*16,file_off,va,fs,ms-fs if i==2 else 1)
  struct.pack_into('<I',h,96+i*4,fs);h[160+i*32:192+i*32]=hashlib.sha256(data).digest();file_off+=fs
 shoff=struct.unpack_from('<Q',elf,40)[0];shsize,shnum=struct.unpack_from('<HH',elf,58)
 for i in range(shnum):
  row=struct.unpack_from('<IIQQQQIIQQ',elf,shoff+i*shsize)
  if row[1]==7:
   note=elf[row[4]:row[4]+row[5]]
   if len(note)>=16:
    ns,ds,typ=struct.unpack_from('<III',note)
    if typ==3 and note[12:16]==b'GNU\0':h[64:64+min(ds,32)]=note[12+((ns+3)&~3):12+((ns+3)&~3)+min(ds,32)]
 nso=bytes(h)+b''.join(sections);(directory/'main').write_bytes(nso)
 files=[('main',nso),('main.npdm',npdm)];strings=b'';entries=b'';offset=0
 for name,data in files:
  entries+=struct.pack('<QQII',offset,len(data),len(strings),0);strings+=name.encode()+b'\0';offset+=len(data)
 strings+=b'\0'*((-(16+len(entries)+len(strings)))%16)
 pfs=struct.pack('<4sIII',b'PFS0',len(files),len(strings),0)+entries+strings+b''.join(data for _,data in files)
 (directory/'exefs.nsp').write_bytes(pfs)
 print('Packaged background NSO:',len(nso),'bytes; ExeFS:',len(pfs),'bytes')
if __name__=='__main__':pack(*sys.argv[1:])
