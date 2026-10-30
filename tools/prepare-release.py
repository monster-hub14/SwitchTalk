from pathlib import Path
import sys,json,hashlib,subprocess,zipfile,shutil
root=Path(__file__).resolve().parents[1];a=root/'app/build';q=root/'overlay-worker/build';out=Path(sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=True);version=(root/'VERSION').read_text().strip();name='SwitchTalk-'+version
report=json.loads((root/'validation-all.json').read_text());assert report['status']=='PASS' and report['version']==version
payload=a/'SwitchTalk.nro';data=payload.read_bytes();assert hashlib.sha256(data).hexdigest()==report['payload_sha256']
assert hashlib.sha256((a/'build-inputs.json').read_bytes()).hexdigest()==report['build_inputs_sha256']
for path,h in json.loads((a/'build-inputs.json').read_text()).items():assert hashlib.sha256((root/path).read_bytes()).hexdigest()==h,path
for path in [root/'validation-app.json',root/'validation-launcher.json',q/'validation-worker.json',q/'validation-controller.json',q/'validation-overlay.json']:assert json.loads(path.read_text())['status']=='PASS'
assets=[name+'.zip',name+'-Source.zip',name+'-Debug.zip','SwitchTalkApp.nro','update.txt',name+'-Manifest.json',name+'-SHA256.txt']
# Payload/update.txt are a stable channel pair only within this new version.
# Keep versioned copies under release/VERSION; then expose user-facing outputs.
release=root/'release'/version
if release.exists():raise SystemExit('Refusing to overwrite a prepared stable release: '+str(release))
release.mkdir(parents=True)
shutil.copy2(payload,release/'SwitchTalkApp.nro');shutil.copy2(a/'update.txt',release/'update.txt');descriptor=(release/'update.txt').read_bytes()
entries={'switch/SwitchTalk/SwitchTalk.nro':(a/'Launcher.nro').read_bytes(),'switch/SwitchTalk/app-a.nro':data,'switch/SwitchTalk/app-a.txt':descriptor,'switch/SwitchTalk/active.txt':b'a\n','switch/SwitchTalk/booting.txt':b'','switch/.overlays/SwitchTalk.ovl':(q/'SwitchTalk.ovl').read_bytes(),'switch/SwitchTalkAlertsTest/SwitchTalkAlertsTest.nro':(q/'SwitchTalkAlertsTest.nro').read_bytes(),'atmosphere/contents/4200000000535755/exefs.nsp':(q/'exefs.nsp').read_bytes()}
for n in ['README.md','INSTALL.txt','UPDATES.md','THIRD_PARTY.md','LICENSE','validation-all.json','validation-app.json','validation-launcher.json','validation-updater-tests.json']:entries[n]=(root/n).read_bytes()
entries['validation/host-tests.txt']=(root/'host-tests.txt').read_bytes()
for part in ['app','overlay-worker']:
 for p in (root/part/'licenses').rglob('*'):
  if p.is_file():entries['licenses/'+part+'/'+p.name]=p.read_bytes()
entries['licenses/app/COPYING.switch-tools']=(root/'app/COPYING.switch-tools').read_bytes()
record={'version':version,'author':'Monster Review','device_tested':False,'automatic_updates_default':True,'automatic_updates_optional':True,'disable':'Full app: ZL Updates, X until AUTOMATIC UPDATES: OFF, B back','setting_persistent':True,'launcher_abi':1,'protocol':'SWTU1','channel':'https://github.com/monster-hub14/SwitchTalk/releases/latest/download/update.txt','install_path':'switch/SwitchTalk/SwitchTalk.nro','first_install':'full ZIP required once for 1.0.x users','host_and_native_validation':'PASS','running_slot_overwritten':False,'background_voice':False,'updater_client_credentials':False,'matched_companions_embedded':True,'previous_versions_preserved':True,'payload_sha256':report['payload_sha256'],'payload_bytes':len(data)}
entries['RELEASE-MANIFEST.json']=json.dumps(record,indent=2).encode()
with zipfile.ZipFile(release/(name+'.zip'),'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
 for path,blob in entries.items():z.writestr(path,blob)
with zipfile.ZipFile(release/(name+'.zip')) as z:
 assert z.testzip() is None
 for path,blob in entries.items():assert z.read(path)==blob
 assert not any(p.endswith(('account.bin','updates.cfg','boot2.flag')) or '4200000000535754' in p for p in z.namelist())
with zipfile.ZipFile(release/(name+'-Source.zip'),'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
 for p in root.rglob('*'):
  rel=p.relative_to(root)
  if rel.as_posix()=='publish-assets.json':continue
  if not p.is_file() or rel.parts[0] in ['release','dist','work','.git'] or '__pycache__' in rel.parts:continue
  if len(rel.parts)>1 and rel.parts[0] in ['app','overlay-worker'] and rel.parts[1]=='build':continue
  if rel.as_posix().startswith('app/romfs/bundle/'):continue
  if p.suffix in ['.pyc','.o','.elf','.map','.nro','.ovl','.nsp'] or p.name in ['account.bin','account.bin.tmp']:continue
  z.write(p,'SwitchTalk-'+version+'/'+rel.as_posix())
with zipfile.ZipFile(release/(name+'-Debug.zip'),'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
 for p in [a/'SwitchTalk.elf',a/'SwitchTalk.map',a/'Launcher.elf',a/'Launcher.map',q/'worker.elf',q/'worker.map',q/'overlay.elf',q/'overlay.map',q/'controller.elf',q/'controller.map']:z.write(p,p.name)
record['assets']={n:hashlib.sha256((release/n).read_bytes()).hexdigest() for n in assets[:5]}
(release/(name+'-Manifest.json')).write_text(json.dumps(record,indent=2))
record['assets'][name+'-Manifest.json']=hashlib.sha256((release/(name+'-Manifest.json')).read_bytes()).hexdigest()
(release/(name+'-SHA256.txt')).write_text(''.join(h+'  '+n+'\n' for n,h in record['assets'].items()))
for n in assets:
 p=release/n
 if p.suffix=='.zip':
  with zipfile.ZipFile(p) as z:assert z.testzip() is None
 target=out/n
 if target.exists():raise SystemExit('Refusing to replace output: '+str(target))
 shutil.copy2(p,target)
(root/'publish-assets.json').write_text(json.dumps({'version':version,'repository':'monster-hub14/SwitchTalk','assets':{n:hashlib.sha256((release/n).read_bytes()).hexdigest() for n in assets}},indent=2))
print(json.dumps(record,indent=2))
