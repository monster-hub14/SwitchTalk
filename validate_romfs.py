"""Independent reader validates switch-tools RomFS metadata and every embedded asset."""
import struct,hashlib
from pathlib import Path
def validate_romfs(data,source):
 h=struct.unpack_from('<10Q',data);assert h[0]==0x50
 dh,ds,dm,dms,fh,fs,fm,fms,blob=h[1:]
 for offset,size in [(dh,ds),(dm,dms),(fh,fs),(fm,fms)]:assert offset>=0x50 and offset+size<=len(data)
 files={};visited=set()
 def directory(off,path):
  assert off not in visited and off+24<=dms;visited.add(off)
  parent,sibling,child,file,hashnext,n=struct.unpack_from('<6I',data,dm+off)
  assert off+24+n<=dms
  while file!=0xffffffff:
   assert file+32<=fms
   par,sib,pos,size,hashnext,n=struct.unpack_from('<IIQQII',data,fm+file)
   assert file+32+n<=fms and blob+pos+size<=len(data)
   name=data[fm+file+32:fm+file+32+n].decode();assert '/' not in name and '..' not in name
   rel=path+name;payload=data[blob+pos:blob+pos+size];assert payload==(source/rel).read_bytes();files[rel]={'size':size,'sha256':hashlib.sha256(payload).hexdigest()};file=sib
  while child!=0xffffffff:
   par,sib,sub,file,hn,n=struct.unpack_from('<6I',data,dm+child);name=data[dm+child+24:dm+child+24+n].decode();assert '/' not in name and '..' not in name
   directory(child,path+name+'/');child=sib
 directory(0,'')
 assert set(files)==set(p.relative_to(source).as_posix() for p in source.rglob('*') if p.is_file())
 return files
