from pathlib import Path
import subprocess,concurrent.futures,shutil
root=Path.cwd();s=root/'work/libnx-updated/libnx-4.12.0/nx';sdk=root/'work/toolchain/opt/devkitpro';b=s/'native-build';b.mkdir(exist_ok=True)
cc=sdk/'devkitA64/bin/aarch64-none-elf-gcc.exe';ar=sdk/'devkitA64/bin/aarch64-none-elf-ar.exe'
flags=['-g','-Os','-ffunction-sections','-fdata-sections','-march=armv8-a+crc+crypto','-mtune=cortex-a57','-mtp=soft','-fPIC','-ftls-model=local-exec','-DNDEBUG=1','-D__SWITCH__','-DLIBNX_NO_DEPRECATION','-I'+str(s/'include'),'-I'+str(s/'external/bsd/include'),'-I'+str(b),'-iquote',str(s/'include/switch')]
(b/'default_font_bin.h').write_text('extern const unsigned char default_font_bin[];\n')
d=(s/'data/default_font.bin').read_bytes();(b/'font.c').write_text('const unsigned char default_font_bin[] __attribute__((aligned(16)))={'+','.join(map(str,d))+'};')
sources=list((s/'source').rglob('*.c'))+list((s/'source').rglob('*.s'))+[b/'font.c']
def compile(p):
 o=b/(p.stem+'.o');q=subprocess.run([str(cc),*flags,'-c',str(p),'-o',str(o)],capture_output=True,text=True)
 if q.returncode:raise RuntimeError(str(p)+'\n'+q.stderr)
 return o
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:objects=list(pool.map(compile,sources))
subprocess.run([str(ar),'rcs',str(b/'libnx.a'),*map(str,objects)],check=True)
backup=sdk/'libnx-4.9-backup'
if not backup.exists():shutil.copytree(sdk/'libnx',backup)
t=sdk/'libnx';shutil.copytree(s/'include',t/'include',dirs_exist_ok=True);shutil.copytree(s/'external/bsd/include',t/'include',dirs_exist_ok=True)
shutil.copy2(b/'libnx.a',t/'lib/libnx.a')
for n in ['switch.ld','switch.specs','switch_rules','default_icon.jpg']:shutil.copy2(s/n,t/n)
print('Installed libnx 4.12.0 from official source:',len(objects),'objects')
