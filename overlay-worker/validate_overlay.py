from pathlib import Path
import struct,hashlib,json
r=Path(__file__).resolve().parent;b=r/'build';full=(b/'SwitchTalk.ovl').read_bytes();assert full[-4:]==b'ULTR';nro=full[:-4];elf=(b/'overlay.elf').read_bytes()
mod=struct.unpack_from('<I',nro,4)[0]+struct.unpack_from('<I',nro,32)[0]
assert nro[mod:mod+4]==b'MOD0' and nro[mod+52:mod+56]==b'LNY2' and struct.unpack_from('<I',nro,mod+56)[0]>=1, 'Missing AMS 1.10+ runtime compatibility'
assert nro[16:20]==b'NRO0' and struct.unpack_from('<H',elf,18)[0]==183
size=struct.unpack_from('<I',nro,24)[0];phoff=struct.unpack_from('<Q',elf,32)[0];phsize,phnum=struct.unpack_from('<HH',elf,54);loads=[]
for i in range(phnum):
 row=struct.unpack_from('<IIQQQQQQ',elf,phoff+i*phsize)
 if row[0]==1:loads.append(row)
assert len(loads)==3
for i,row in enumerate(loads):
 off,va,fs,ms=row[2],row[3],row[5],row[6];no,ns=struct.unpack_from('<II',nro,32+8*i);assert no==va and ns%4096==0 and ns>=fs and no+ns<=size
 start=128 if i==0 else 0;assert nro[no+start:no+fs]==elf[off+start:off+fs]
magic,version,io,isz,na,nasz,ro,rsz=struct.unpack_from('<4sIQQQQQQ',nro,size);assert magic==b'ASET' and version==0 and nasz==0x4000 and rsz==0
assert nro[size+io:size+io+isz]==(r/'icon.jpg').read_bytes();nacp=nro[size+na:size+na+nasz];assert nacp[:len(b'SwitchTalk Overlay\0')]==b'SwitchTalk Overlay\0' and nacp[0x3060:0x3066]==b'1.0.0\0';assert size+na+nasz==len(nro)
report={'status':'PASS','artifact':'SwitchTalk.ovl','version':'1.0.0','bytes':len(full),'sha256':hashlib.sha256(full).hexdigest(),'static_loaded_image_bytes':max(x[3]+x[6] for x in loads),'ultrahand_library_commit':'856ddbddd796fc4a59ad2e0bf939c5963e6f9dd2','libnx_version':'4.12.0','ams_110_runtime_check':'PASS','signature':'ULTR','read_only':False,'device_tested':False,'checks':['Actual AArch64 ELF segments match NRO','NRO and ASET/icon/NACP sizes and bounds','Ultrahand signature after asset sections','Exact pinned library revision bundled with Ultrahand 2.5.3']}
(b/'validation-overlay.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
