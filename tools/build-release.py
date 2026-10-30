from pathlib import Path
import subprocess,sys,json,hashlib,shutil
root=Path(__file__).resolve().parents[1];sdk=Path(sys.argv[1]).resolve();tcc=Path(sys.argv[2]).resolve();q=root/'overlay-worker';a=root/'app'
(a/'romfs/bundle').mkdir(parents=True,exist_ok=True)
for script,args in [('build.py',[sdk,tcc]),('build_overlay.py',[sdk])]:subprocess.run([sys.executable,str(q/script),*map(str,args)],check=True)
entries=[('SwitchTalk.ovl',q/'build/SwitchTalk.ovl'),('exefs.nsp',q/'build/exefs.nsp'),('SwitchTalkWorker.nro',q/'build/SwitchTalkAlertsTest.nro')]
version=(root/'VERSION').read_text().strip();lines=['SWITCHTALK-BUNDLE-1','version='+version]
for name,path in entries:
 data=path.read_bytes();shutil.copy2(path,a/'romfs/bundle'/name);lines.append(name+' '+str(len(data))+' '+hashlib.sha256(data).hexdigest())
(a/'romfs/bundle/files.txt').write_text('\n'.join(lines)+'\n',encoding='ascii',newline='\n')
shell=shutil.which('pwsh') or shutil.which('powershell');assert shell
subprocess.run([shell,'-NoProfile','-File',str(a/'build-windows.ps1'),'-DevkitPro',str(sdk),'-Python',sys.executable,'-OutputDirectory',str(a/'build')],check=True)
print('Built matched app bundle, stable launcher, overlay and worker:',version)

inputs={}
for folder in [a/'source',a/'include',a/'romfs']:
 for p in folder.rglob('*'):
  if p.is_file():inputs[p.relative_to(root).as_posix()]=hashlib.sha256(p.read_bytes()).hexdigest()
for p in [a/'icon.jpg',a/'package_nro.py',a/'build-windows.ps1',a/'vendor/rtc/build/librtc-switch.a']:
 inputs[p.relative_to(root).as_posix()]=hashlib.sha256(p.read_bytes()).hexdigest()
(a/'build/build-inputs.json').write_text(json.dumps(inputs,indent=2))
