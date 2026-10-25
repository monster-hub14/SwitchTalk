from pathlib import Path
import struct,hashlib,json
root=Path(__file__).resolve().parent;d=root/'build';nso=(d/'main').read_bytes();elf=(d/'worker.elf').read_bytes();npdm=(d/'main.npdm').read_bytes();pfs=(d/'exefs.nsp').read_bytes()
assert nso[:4]==b'NSO0' and struct.unpack_from('<I',nso,12)[0]==0x38
phoff=struct.unpack_from('<Q',elf,32)[0];phsize,phnum=struct.unpack_from('<HH',elf,54);loads=[]
for i in range(phnum):
 row=struct.unpack_from('<IIQQQQQQ',elf,phoff+i*phsize)
 if row[0]==1:loads.append(row)
assert len(loads)==3
resident=0
for i,load in enumerate(loads):
 off,va,fs,ms=load[2],load[3],load[5],load[6];no,nva,nfs,extra=struct.unpack_from('<IIII',nso,16+i*16)
 assert nva==va and nfs==fs and nso[no:no+nfs]==elf[off:off+fs]
 assert hashlib.sha256(nso[no:no+nfs]).digest()==nso[160+i*32:192+i*32]
 assert struct.unpack_from('<I',nso,96+i*4)[0]==fs
 if i==2:assert extra==ms-fs
 resident=max(resident,va+ms)
assert npdm[:4]==b'META';aci_off,aci_size,acid_off,acid_size=struct.unpack_from('<IIII',npdm,0x70)
assert aci_off+aci_size==len(npdm) and acid_off+acid_size<=len(npdm)
aci=npdm[aci_off:aci_off+aci_size];acid=npdm[acid_off:acid_off+acid_size]
fah_off=struct.unpack_from('<I',aci,0x20)[0];fac_off=struct.unpack_from('<I',acid,0x220)[0]
assert struct.unpack_from('<Q',aci,fah_off+4)[0]==0x200000
assert struct.unpack_from('<Q',acid,fac_off+4)[0]==0x200000
assert aci[:4]==b'ACI0' and acid[0x200:0x204]==b'ACID'
tid=0x4200000000535755;assert struct.unpack_from('<Q',aci,0x10)[0]==tid and struct.unpack_from('<QQ',acid,0x210)==(tid,tid)
assert npdm[0xf]==3 and struct.unpack_from('<I',npdm,0x1c)[0]==0x8000
assert (struct.unpack_from('<I',acid,0x20c)[0]>>2)&3==2
sac_off,sac_size,kac_off,kac_size=struct.unpack_from('<IIII',aci,0x28)
sac=aci[sac_off:sac_off+sac_size];services=[];pos=0
while pos<len(sac):
 ctrl=sac[pos];size=(ctrl&7)+1;services.append((bool(ctrl&0x80),sac[pos+1:pos+1+size].decode()));pos+=size+1
expected=json.loads((root/"sysmodule/SwitchTalk.json").read_text());assert set(services)==set([(False,s) for s in expected["service_access"]]+[(True,s) for s in expected["service_host"]])
assert sac_size>0

ka_off,ka_size=struct.unpack_from('<II',acid,0x230);kac=aci[kac_off:kac_off+kac_size];assert kac==acid[ka_off:ka_off+ka_size]
svcs=set()
for i in range(0,len(kac),4):
 word=struct.unpack_from('<I',kac,i)[0]
 if word&31==15:
  group=word>>29
  for bit in range(24):
   if (word>>(bit+5))&1:svcs.add(group*24+bit)
assert {0x8,0x9,0x15,0x18,0x1f,0x21,0x41,0x43,0x71}.issubset(svcs)
assert pfs[:4]==b'PFS0';n,ss=struct.unpack_from('<II',pfs,4);assert n==2
strings=pfs[16+n*24:16+n*24+ss];start=16+n*24+ss;files={}
for i in range(n):
 off,size,no,res=struct.unpack_from('<QQII',pfs,16+i*24);name=strings[no:].split(b'\0',1)[0].decode();files[name]=pfs[start+off:start+off+size];assert len(files[name])==size
assert files=={'main':nso,'main.npdm':npdm}
report={'status':'PASS','program_id':f'{tid:016X}','service_port':'swtprb','connection_route':'Atmosphere SM registered service','protocol':10,'filesystem_permission':'SD card only (bit 21)','nso_bytes':len(nso),'exefs_bytes':len(pfs),'static_image_bytes_including_1536KiB_heap':resident,'exefs_sha256':hashlib.sha256(pfs).hexdigest(),'checks':['NSO segments match actual AArch64 ELF','all three NSO hashes and uncompressed flags','NPDM identity/range, system pool, CPU and stack','NPDM service ACL and required IPC/syscall capabilities','nonempty service access control required by SM RegisterProcess','ACID/ACI0 capability agreement','PFS0 directory and exact embedded NSO/NPDM'],'device_tested':False}
(root/'build/validation-worker.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))

