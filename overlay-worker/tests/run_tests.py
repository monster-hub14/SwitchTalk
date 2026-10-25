from pathlib import Path
import ctypes,sys,json
r=Path(__file__).resolve().parent.parent;tcc=Path(sys.argv[1]).resolve();sdk=Path(sys.argv[2]).resolve();out=r/'build';out.mkdir(exist_ok=True);notify=(out/'notification-fixtures').resolve();notify.mkdir(exist_ok=True)
c=ctypes.CDLL(str(tcc/'libtcc.dll'));P=ctypes.c_void_p;S=ctypes.c_char_p;I=ctypes.c_int
for n,a,ret in [('tcc_new',[],P),('tcc_set_lib_path',[P,S],None),('tcc_add_include_path',[P,S],I),('tcc_define_symbol',[P,S,S],None),('tcc_set_output_type',[P,I],I),('tcc_add_file',[P,S],I),('tcc_run',[P,I,P],I),('tcc_delete',[P],None)]:
 f=getattr(c,n);f.argtypes=a;f.restype=ret
cases=[('reply-editor',['tests/test_reply_editor.c']),('qr-service',['source/qr_service.c','source/alert_policy.c','tests/test_qr_service.c']),('policy',['source/alert_policy.c','tests/test_policy.c']),('files',['source/notify_file.c','tests/windows_dirent.c','tests/test_notify.c']),('preview',['source/alert_preview.c','source/notify_file.c','tests/windows_dirent.c','tests/test_preview.c']),('group-preview',['source/alert_policy.c','source/alert_preview.c','source/notify_file.c','tests/windows_dirent.c','tests/test_group_preview.c']),('fetch',['source/json.c','source/rich_text.c','source/talk.c','tests/test_fetch.c'])]
for name,files in cases:
 if name=='preview':
  for p in notify.glob('*.notify'):p.unlink()
 state=c.tcc_new();c.tcc_set_lib_path(state,str(tcc).encode())
 for k,v in [('QR_HOST','1'),('ALERT_HOST','1'),('CURL_STATICLIB','1'),('NOTIFY_DIR','"'+notify.as_posix()+'"')]:c.tcc_define_symbol(state,k.encode(),v.encode())
 for inc in [r/'tests/file-shim',r/'tests/transport-shim',r/'include',sdk/'portlibs/switch/include',tcc/'include',tcc/'include/winapi']:assert c.tcc_add_include_path(state,str(inc).encode())==0
 assert c.tcc_set_output_type(state,1)==0
 for f in files:assert c.tcc_add_file(state,str(r/f).encode())==0,f
 assert c.tcc_run(state,0,None)==0,name;c.tcc_delete(state);print(name+': PASS')
 if name=='preview':
  previews=[json.loads(p.read_text(encoding='utf-8')) for p in notify.glob('*.notify')]
  assert len(previews)==2 and any(x['text']=='Alice\nHey, ready to play?' for x in previews)
  assert any('\u4f60\u597d' in x['text'] and '\U0001f600' in x['text'] for x in previews)

files=list(notify.glob('*.notify'));assert len(files)==2 and not list(notify.glob('*.tmp'))
data=[json.loads(p.read_text(encoding="utf-8")) for p in files];assert all(x['title']=='SwitchTalk' and x['duration']==5000 for x in data)
assert all(x['text']=='Friends / Alice\nNew group message' for x in data)
print('Actual JSON output escaping, bounded queue, atomic publication, duplicate suppression: PASS')
