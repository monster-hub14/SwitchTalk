"""Run saved-login tests with real Mbed TLS AES-GCM and mocked Switch key services."""
import ctypes,sys
from pathlib import Path
root=Path(__file__).resolve().parent.parent
storage=Path('work/test-saved-login').resolve();storage.mkdir(parents=True,exist_ok=True)
tcc=Path(sys.argv[1]).resolve();c=ctypes.CDLL(str(tcc/'libtcc.dll'));P=ctypes.c_void_p;S=ctypes.c_char_p;I=ctypes.c_int
for name,args,ret in [('tcc_new',[],P),('tcc_set_lib_path',[P,S],None),('tcc_add_include_path',[P,S],I),('tcc_define_symbol',[P,S,S],None),('tcc_set_output_type',[P,I],I),('tcc_add_file',[P,S],I),('tcc_run',[P,I,P],I),('tcc_delete',[P],None)]:
 f=getattr(c,name);f.argtypes=args;f.restype=ret
state=c.tcc_new();c.tcc_set_lib_path(state,str(tcc).encode())
c.tcc_define_symbol(state,b'MBEDTLS_CONFIG_FILE',b'"crypto_config.h"');c.tcc_define_symbol(state,b'SAVED_LOGIN_HOST',b'1');c.tcc_define_symbol(state,b'SAVED_LOGIN_DIR',('"'+storage.as_posix()+'"').encode())
for inc in [root/'tests/storage-host',root/'tests/host-shim',root/'include',root/'vendor/rtc/mbedtls-2.28.10/include',tcc/'include',tcc/'include/winapi']:assert c.tcc_add_include_path(state,str(inc).encode())==0
assert c.tcc_set_output_type(state,1)==0
files=[root/'vendor/rtc/mbedtls-2.28.10/library'/n for n in ['aes.c','gcm.c','cipher.c','cipher_wrap.c','platform.c','platform_util.c','constant_time.c']]+[root/n for n in ['source/server_url.c','source/saved_login.c','tests/test_saved_login.c']]
for f in files:assert c.tcc_add_file(state,str(f).encode())==0,f
result=c.tcc_run(state,0,None);c.tcc_delete(state);sys.exit(result)


