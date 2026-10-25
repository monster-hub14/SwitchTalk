from pathlib import Path
import subprocess,sys,os,concurrent.futures
r=Path(__file__).resolve().parent;sdk=Path(sys.argv[1]).resolve();out=r/'build';out.mkdir(exist_ok=True);os.environ['DEVKITPRO']=sdk.as_posix();v=r/'vendor/libultrahand'
cc=sdk/'devkitA64/bin/aarch64-none-elf-gcc.exe';cxx=sdk/'devkitA64/bin/aarch64-none-elf-g++.exe'
arch=['-march=armv8-a+crc+crypto','-mtune=cortex-a57','-mtp=soft','-fPIE']
flags=['-g','-Os','-ffunction-sections','-fdata-sections','-D__SWITCH__',*arch]
for p in [v/'common',v/'libtesla/include',v/'libultra/include',sdk/'libnx/include',sdk/'portlibs/switch/include']:flags+=['-isystem',str(p)]
flags+=['-I'+str(r/'include')]
cpp=['-std=gnu++26','-fno-exceptions','-fno-rtti','-fno-unwind-tables','-fno-asynchronous-unwind-tables']
sources=[r/'source/overlay.cpp',v/'common/cJSON.c',*sorted((v/'libultra/source').glob('*.cpp')),*sorted((v/'libtesla/source').glob('*.cpp'))]
objects=[];jobs=[]
for s in sources:
 obj=out/('ovl_'+s.stem+'.o');objects.append(obj)
 if obj.exists() and obj.stat().st_mtime>s.stat().st_mtime:continue
 jobs.append((s,obj))
def compile(job):
 s,obj=job;cmd=[str(cc if s.suffix=='.c' else cxx),*flags,*([] if s.suffix=='.c' else cpp),'-c',str(s),'-o',str(obj)]
 p=subprocess.run(cmd,capture_output=True,text=True)
 if p.returncode:print(p.stdout+p.stderr,flush=True);raise RuntimeError(str(s))
 print('Overlay compiled '+s.name,flush=True)
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:list(pool.map(compile,jobs))
subprocess.run([str(cxx),'-specs='+str(sdk/'libnx/switch.specs'),*arch,'-Wl,--gc-sections','-Wl,-Map,'+str(out/'overlay.map'),*map(str,objects),'-L'+str(sdk/'portlibs/switch/lib'),'-L'+str(sdk/'libnx/lib'),'-lcurl','-lz','-lminizip','-lmbedtls','-lmbedx509','-lmbedcrypto','-lnx','-lm','-o',str(out/'overlay.elf')],check=True)
from package_nro import package
package(out/'overlay.elf',r/'icon.jpg',out/'SwitchTalk.ovl')
with (out/'SwitchTalk.ovl').open('ab') as f:f.write(b'ULTR')
