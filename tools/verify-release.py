from pathlib import Path
import sys,subprocess,os,json,hashlib
root=Path(__file__).resolve().parents[1];sdk=Path(sys.argv[1]).resolve();tcc=Path(sys.argv[2]).resolve();compiler=Path(sys.argv[3]).resolve();a=root/'app';q=root/'overlay-worker';env=os.environ.copy();env['PATH']=str(compiler.parent)+os.pathsep+env['PATH'];env['SWITCHTALK_PREVIEW_DIR']=str(a/'build');logs=[]
def run(args):
 p=subprocess.run(list(map(str,args)),env=env,capture_output=True,text=True,check=True);logs.append(p.stdout);print(p.stdout[-350:].strip());return p
for script in ['validate_app.py','validate-launcher.py']:run([sys.executable,root/script])
run([sys.executable,q/'verify.py',sdk,tcc])
run([sys.executable,a/'tests/run_windows_tests.py',tcc,sdk/'portlibs/switch/include'])
run([sys.executable,root/'tools/run-update-tests.py',compiler])
version=(root/'VERSION').read_text().strip();payload=a/'build/SwitchTalk.nro';data=payload.read_bytes();sha=hashlib.sha256(data).hexdigest()
desc='SWITCHTALK-UPDATE-1\nversion='+version+'\nsize='+str(len(data))+'\nsha256='+sha+'\nprotocol=SWTU1\nasset=SwitchTalkApp.nro\nmin_launcher=1\n'
(a/'build/update.txt').write_text(desc,encoding='ascii',newline='\n')
exe=a/'build/update-verify-host.exe';run([compiler,'-std=c++17','-O2','-Wall','-Wextra','-Werror','-I'+str(a/'include'),a/'tests/test_update_verify.cpp','-o',exe]);run([exe,payload,a/'build/update.txt'])
inputs=json.loads((a/'build/build-inputs.json').read_text())
for name,h in inputs.items():assert hashlib.sha256((root/name).read_bytes()).hexdigest()==h,'Source changed after build: '+name
(root/'host-tests.txt').write_text('\n'.join(logs),encoding='utf-8')
report={'status':'PASS','version':version,'device_tested':False,'payload_sha256':sha,'build_inputs_sha256':hashlib.sha256((a/'build/build-inputs.json').read_bytes()).hexdigest(),'checks':['existing chat/call/avatar/mic/network GUI host suites','new Updates renderer and touch controls','in-flight user send blocks activation even after leaving chat','updater hash/version/title/size/TLS/failure/preferences/idle/rollback suites','matched companion verification and interrupted-install repair','actual client verifier accepts compiled app/update descriptor','app/launcher/RomFS/worker/controller/overlay native structure','build input hash matches source'],'automatic_default':True,'settings_persistent':True}
(root/'validation-all.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
