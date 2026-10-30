from pathlib import Path
import subprocess,sys,os,json
root=Path(__file__).resolve().parents[1];compiler=Path(sys.argv[1]).resolve();env=os.environ.copy();env['PATH']=str(compiler.parent)+os.pathsep+env['PATH'];out=root/'app/build';out.mkdir(exist_ok=True);results=[]
for name in ['updater','update_bundle']:
 exe=out/(name+'-host.exe');cmd=[str(compiler),'-std=c++17','-O2','-Wall','-Wextra','-Werror','-I'+str(root/'app/tests/update-fake'),'-I'+str(root/'app/include'),str(root/'app/tests'/('test_'+name+'.cpp')),'-o',str(exe)]
 subprocess.run(cmd,check=True,env=env);p=subprocess.run([str(exe)],capture_output=True,text=True,check=True,env=env);print(p.stdout.strip());results.append({'suite':name,'status':'PASS','output':p.stdout.strip()})
(root/'validation-updater-tests.json').write_text(json.dumps({'status':'PASS','device_tested':False,'suites':results},indent=2))
