from pathlib import Path
import subprocess,sys,json,hashlib
r=Path(__file__).resolve().parent;root=r.parent.parent;sdk=Path(sys.argv[1]).resolve();tcc=Path(sys.argv[2]).resolve();logs=[]
commands=[[r/'test.py',tcc],[r/'tests/run_tests.py',tcc,sdk],[r/'validate_worker.py'],[r/'validate_controller.py'],[r/'validate_overlay.py']]
for args in commands:
 p=subprocess.run([sys.executable,*map(str,args)],check=True,capture_output=True,text=True);logs.append(p.stdout)
(r/'build/host-tests.txt').write_text('\n'.join(logs));print('All controller, policy, filesystem, API, NSO/NPDM and NRO checks: PASS')

manifest=json.loads((r/'sysmodule/SwitchTalk.json').read_text());assert manifest['filesystem_access']['permissions']=='0x200000' and manifest['service_host']==['swtprb']
worker=(r/'source/worker.c').read_text();transport=(r/'source/talk.c').read_text();controller=(r/'source/controller.c').read_text()
assert 'svcManageNamedPort(' not in worker and 'smRegisterService(' in worker
assert 'noStatusUpdate=1' in transport and 'lookIntoFuture=1' not in transport and 'setReadMarker=0' in transport and 'markNotificationsAsRead=0' in transport
assert 'HidNpadButton_R' in controller and 'PROBE_TEST' in controller
(r/'build/integration-validation.json').write_text(json.dumps({'transport':'room summaries, recent history and explicit message sending','connection':'SM-registered service','filesystem_permission':'SD card only','manual_start_only':True,'inline_status_protocol':10,'stable_app_not_part_of_this_build':False,'device_tested':False},indent=2))

assert "PROBE_OVERRIDE" not in (r/"include/probe.h").read_text()
assert "ignore_settings" not in (r/"source/alert_policy.c").read_text()
assert "HidNpadButton_L" not in controller

assert "alert_override" not in (r/"source/alert_worker.c").read_text()
assert "PROBE_OVERRIDE" not in (r/"source/worker.c").read_text()

import subprocess
nm=sdk/'devkitA64/bin/aarch64-none-elf-nm.exe'
symbols=subprocess.run([str(nm),'-g',str(r/'build/worker.elf')],capture_output=True,text=True,check=True).stdout
assert ' qr_http_history' in symbols and ' qr_http_send' in symbols
assert '#define QR_ENABLE_SEND 1' in (r/'include/quick_reply.h').read_text()
(r/'build/reply-validation.json').write_text(json.dumps({'status':'PASS','history_HTTP_code_linked':True,'send_HTTP_code_linked':True,'draft_and_send_RPC':'host tested','mapped_IPC_buffers':False,'device_tested':False},indent=2))

# Reject runtime regressions in every shipped binary, not just the overlay.
import struct
for name in ['worker','controller','overlay']:
 elf=(r/'build'/(name+'.elf')).read_bytes()
 phoff=struct.unpack_from('<Q',elf,32)[0];phsize,phnum=struct.unpack_from('<HH',elf,54)
 for i in range(phnum):
  row=struct.unpack_from('<IIQQQQQQ',elf,phoff+i*phsize)
  if row[0]==1 and row[3]==0:
   data=elf[row[2]:row[2]+row[5]]
   mod=struct.unpack_from('<I',data,4)[0]
   assert data[mod:mod+4]==b'MOD0' and data[mod+52:mod+56]==b'LNY2' and struct.unpack_from('<I',data,mod+56)[0]>=1,name
   break
 else:raise AssertionError(name+' has no entry segment')
print('AMS 1.10+ runtime extension verified in worker, controller and overlay: PASS')
