from pathlib import Path
import struct, hashlib, json
from PIL import Image
root=Path(__file__).resolve().parent
nro=(root/'build/SwitchTalkAlertsTest.nro').read_bytes()
elf=(root/'build/controller.elf').read_bytes()
assert nro[16:20]==b'NRO0'
version,size,flags=struct.unpack_from('<III',nro,20)
assert version==0 and flags==0
phoff=struct.unpack_from('<Q',elf,32)[0]
phsize,phnum=struct.unpack_from('<HH',elf,54)
loads=[]
for i in range(phnum):
    typ,fl,off,va,pa,fs,ms,al=struct.unpack_from('<IIQQQQQQ',elf,phoff+i*phsize)
    if typ==1: loads.append((off,va,fs,ms))
segments=[]
for i,(off,va,fs,ms) in enumerate(loads):
    no,ns=struct.unpack_from('<II',nro,32+8*i)
    assert no==va and ns%4096==0 and ns>=fs and no+ns<=size
    start=128 if i==0 else 0
    assert nro[no+start:no+fs]==elf[off+start:off+fs]
    assert not any(nro[no+fs:no+ns])
    segments.append({'offset':no,'size':ns})
assert segments[0]['offset']==0
assert all(segments[i]['offset']+segments[i]['size']==segments[i+1]['offset'] for i in range(2))
assert segments[-1]['offset']+segments[-1]['size']==size
bss=struct.unpack_from('<I',nro,56)[0]
assert segments[2]['size']+bss>=loads[2][3]
magic,ver,io,isz,na,nasz,ro,rsz=struct.unpack_from('<4sIQQQQQQ',nro,size)
assert magic==b'ASET' and ver==0 and nasz==0x4000 and rsz==0
assert nro[size+io:size+io+isz]==(root/'icon.jpg').read_bytes()
nacp=nro[size+na:size+na+nasz]
assert nacp[:len(b'SwitchTalk Worker\0')]==b'SwitchTalk Worker\0'
assert nacp[0x3060:0x3066]==b'1.0.1\0'
assert size+na+nasz==len(nro)
icon=Image.open(root/'icon.jpg')
assert icon.size==(256,256) and icon.format=='JPEG'
for language in range(12):
 assert nacp[language*0x300+0x200:language*0x300+0x200+15]==b'Monster Review\0'
report={'status':'PASS','author':'Monster Review','size_bytes':len(nro),'sha256':hashlib.sha256(nro).hexdigest(),'segments':segments,'bss_size':bss,'build_id':nro[64:96].hex(),'icon':'256x256 JPEG','title':'SwitchTalk Worker','version':'1.0.1','checks':['AArch64 ELF linked','NRO0 and ASET headers','all executable segments match linked ELF except required NRO header','segment alignment, bounds, padding and BSS','embedded JPEG and NACP metadata'],'device_tested':False}
(root/'build/validation-controller.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))

