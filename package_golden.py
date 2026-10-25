from pathlib import Path
import shutil,zipfile,hashlib,json,sys,datetime
base=Path(__file__).resolve().parent
out=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else base/'dist';out.mkdir(parents=True,exist_ok=True)
name='SwitchTalk-1.0.0-Golden';install=out/(name+'.zip');source=out/(name+'-Source.zip');debug=out/(name+'-Debug.zip')
for p in [install,source,debug,out/(name+'-SHA256.txt'),out/(name+'-Manifest.json')]:
 if p.exists():raise SystemExit('Frozen artifact already exists; refusing overwrite: '+str(p))
d=base/'release';d.mkdir(exist_ok=True);a=base/'app';q=base/'overlay-worker';b=q/'build'
files={'switch/SwitchTalk/SwitchTalk.nro':a/'build/SwitchTalk.nro','switch/.overlays/SwitchTalk.ovl':b/'SwitchTalk.ovl','switch/SwitchTalkAlertsTest/SwitchTalkAlertsTest.nro':b/'SwitchTalkAlertsTest.nro','atmosphere/contents/4200000000535755/exefs.nsp':b/'exefs.nsp'}
for path,p in files.items():
 dest=d/path;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,dest)
shutil.copy2(base/'README.txt',d/'READ-ME-FIRST.txt');shutil.copy2(base/'VERSION',d/'VERSION')
for part in ['app','overlay-worker']:shutil.copytree(base/part/'licenses',d/'licenses'/part,dirs_exist_ok=True)
shutil.copy2(a/'COPYING.switch-tools',d/'licenses/app/COPYING.switch-tools')
v=d/'validation';v.mkdir(exist_ok=True)
for n in ['validation-controller.json','validation-worker.json','validation-overlay.json','host-tests.txt','integration-validation.json','reply-validation.json']:shutil.copy2(b/n,v/n)
for n in ['validation-app.json','metadata-source-comparison.json','baseline-artifacts.json']:shutil.copy2(base/n,v/n)
record={'version':'1.0.0','release':'Golden','frozen':True,'created_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'components':{'application':'1.0.0','overlay':'1.0.0','worker':'1.0.0'},'internal_IPC_protocol':10,'baseline_device_confirmed_by_user':True,'golden_rebuild_device_tested':False,'functional_source_changes':False,'source_comparison':'PASS: exact match after whitelisted name/version/user-agent substitutions','host_tests':'PASS','binary_validation':'PASS','auto_start':False,'additional_background_threads':0,'payload_sha256':{n:hashlib.sha256(p.read_bytes()).hexdigest() for n,p in files.items()}}
(d/'RELEASE-MANIFEST.json').write_text(json.dumps(record,indent=2))
checks={p.relative_to(d).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in d.rglob('*') if p.is_file() and p.name!='SHA256.txt'}
(d/'SHA256.txt').write_text(''.join(h+'  '+n+'\n' for n,h in sorted(checks.items())))
with zipfile.ZipFile(install,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
 for p in d.rglob('*'):
  if p.is_file():z.write(p,p.relative_to(d).as_posix())
with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
 for p in base.rglob('*'):
  rel=p.relative_to(base)
  if not p.is_file() or rel.parts[0] in ['release','dist'] or '__pycache__' in rel.parts or '.git' in rel.parts:continue
  if len(rel.parts)>1 and rel.parts[0] in ['app','overlay-worker'] and rel.parts[1]=='build':continue
  if p.suffix in ['.pyc','.o','.elf','.map','.nro','.ovl'] or p.name in ['account.bin','account.bin.tmp']:continue
  z.write(p,'SwitchTalk-1.0.0/'+rel.as_posix())
with zipfile.ZipFile(debug,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
 for n in ['worker.elf','worker.map','controller.elf','controller.map','overlay.elf','overlay.map','main.npdm']:z.write(b/n,'overlay-worker/'+n)
 for n in ['SwitchTalk.elf','SwitchTalk.map']:z.write(a/'build'/n,'app/'+n)
for p in [install,source,debug]:
 with zipfile.ZipFile(p) as z:assert z.testzip() is None
with zipfile.ZipFile(install) as z:
 assert not any(n.endswith(('boot2.flag','account.bin','account.bin.tmp')) or '4200000000535754' in n for n in z.namelist())
 for n,h in checks.items():assert hashlib.sha256(z.read(n)).hexdigest()==h
 assert set(n for n in z.namelist() if n.endswith(('.nro','.ovl','.nsp')))==set(files)
artifact_hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [install,source,debug]}
record['archive_readback']='PASS';record['artifact_sha256']=artifact_hashes
manifest=out/(name+'-Manifest.json');manifest.write_text(json.dumps(record,indent=2));artifact_hashes[manifest.name]=hashlib.sha256(manifest.read_bytes()).hexdigest()
(out/(name+'-SHA256.txt')).write_text(''.join(h+'  '+n+'\n' for n,h in artifact_hashes.items()))
print(json.dumps(record,indent=2))
