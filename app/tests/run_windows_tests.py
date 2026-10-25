"""Run native C tests on Windows using TinyCC's libtcc.dll.
Usage: python tests/run_windows_tests.py <tcc-directory> <curl-include-directory>
Network and libnx calls are mocked; no server access or credentials required.
"""
import ctypes,sys,os
from pathlib import Path
root=Path(__file__).resolve().parent.parent
preview=Path(os.environ.get('SWITCHTALK_PREVIEW_DIR','work/release-16')).resolve();preview.mkdir(parents=True,exist_ok=True)
os.environ['SWITCHTALK_PREVIEW_DIR']=str(preview)
tcc=Path(sys.argv[1]).resolve();curl=Path(sys.argv[2]).resolve()
c=ctypes.CDLL(str(tcc/'libtcc.dll'))
P=ctypes.c_void_p;S=ctypes.c_char_p;I=ctypes.c_int
for name,args,ret in [('tcc_new',[],P),('tcc_set_lib_path',[P,S],None),('tcc_add_include_path',[P,S],I),('tcc_define_symbol',[P,S,S],None),('tcc_set_output_type',[P,I],I),('tcc_add_file',[P,S],I),('tcc_run',[P,I,P],I),('tcc_delete',[P],None)]:
 f=getattr(c,name);f.argtypes=args;f.restype=ret
for files in [('source/json.c','source/rich_text.c','tests/test_rich_text.c'),('source/avatars.c','source/avatar_image.c','tests/test_avatars.c'),('source/avatar_http.c','tests/test_avatar_http.c'),('source/call_state.c','source/call_view.c','tests/test_foreground.c'),('source/gui.c','source/avatars.c','source/avatar_image.c','tests/test_gui.c'),('source/cJSON.c','source/call_http.c','tests/test_call_http.c'),('source/cJSON.c','source/call_protocol.c','tests/test_call_protocol.c'),('source/pcm.c','source/mic_input.c','tests/test_mic_input.c'),('source/pcm.c','tests/test_pcm.c'),('source/json.c','tests/test_json.c'),('source/messages.c','tests/test_messages.c'),('source/server_url.c','source/json.c','source/messages.c','source/rich_text.c','source/talk.c','tests/test_talk_api.c'),('source/messages.c','source/network.c','tests/test_network.c')]:
 state=c.tcc_new();c.tcc_set_lib_path(state,str(tcc).encode())
 c.tcc_define_symbol(state,b'CURL_STATICLIB',b'1')
 c.tcc_define_symbol(state,b'STBI_NO_SIMD',b'1')
 if 'tests/test_gui.c' in files:
  c.tcc_define_symbol(state,b'GUI_HEADLESS',b'1')
  c.tcc_define_symbol(state,b'STBI_NO_SIMD',b'1')
  c.tcc_define_symbol(state,b'GUI_PREVIEW_DIR',('"'+preview.as_posix()+'"').encode())
 for inc in ([root/'tests/ui-shim'] if 'tests/test_foreground.c' in files or 'tests/test_gui.c' in files else [])+([root/'tests/mic-shim'] if 'tests/test_mic_input.c' in files else [])+[root/'tests/host-shim',root/'include',curl,tcc/'include',tcc/'include/winapi']:
  assert c.tcc_add_include_path(state,str(inc).encode())==0
 assert c.tcc_set_output_type(state,1)==0
 for f in files:assert c.tcc_add_file(state,str(root/f).encode())==0,f
 result=c.tcc_run(state,0,None)
 c.tcc_delete(state)
 if result:sys.exit(result)
 if 'tests/test_foreground.c' in files:print('Call controls: controller/touch mute, hangup, exit, repeated calls, worker start failure and cleanup before network resume: PASS')
 if 'tests/test_gui.c' in files:print('Native GUI renderer: guarded framebuffer, login password hiding, touch hit testing, pagination, busy controls, long/empty/error messages, call and microphone screens: PASS')
print('All native C test suites passed.')
