"""Run real Mbed TLS + ported DTLS initialization on Windows using TinyCC DLL.
Usage: python tests/run_windows_rtc_tests.py /path/to/tcc
Hardware entropy is deterministic/test-only; sockets and SRTP are not exercised.
This does not emulate a Switch or perform a WebRTC handshake.
"""
import ctypes,sys
from pathlib import Path
root=Path(__file__).resolve().parent.parent
rtc=root/'vendor/rtc';host=root/'tests/rtc-host';tcc=Path(sys.argv[1]).resolve()
peer=next(d for d in rtc.glob('libpeer-*') if d.is_dir())
c=ctypes.CDLL(str(tcc/'libtcc.dll'))
P=ctypes.c_void_p;S=ctypes.c_char_p;I=ctypes.c_int
for name,args,ret in [('tcc_new',[],P),('tcc_set_lib_path',[P,S],None),('tcc_add_include_path',[P,S],I),('tcc_set_output_type',[P,I],I),('tcc_add_file',[P,S],I),('tcc_run',[P,I,P],I),('tcc_delete',[P],None)]:
 f=getattr(c,name);f.argtypes=args;f.restype=ret
c.tcc_define_symbol.argtypes=[P,S,S];c.tcc_define_symbol.restype=None
state=c.tcc_new();c.tcc_define_symbol(state,b'MBEDTLS_CONFIG_FILE',b'"dtls_host_config.h"');c.tcc_define_symbol(state,b'CONFIG_MBEDTLS_2_X',b'1');c.tcc_define_symbol(state,b'CONFIG_USE_USRSCTP',b'0');c.tcc_define_symbol(state,b'CONFIG_DTLS_USE_ECDSA',b'1');c.tcc_define_symbol(state,b'LOG_LEVEL',b'-1');c.tcc_define_symbol(state,b'CURL_STATICLIB',b'1');c.tcc_set_lib_path(state,str(tcc).encode())
c.tcc_set_options.argtypes=[P,S];c.tcc_set_options.restype=None
c.tcc_set_options(state,('-include "'+str(host/'host_time.h').replace('\\','/')+'"').encode())
for inc in [host,rtc/'mbedtls-2.28.10/include',peer/'src',rtc/'libsrtp-2.7.0',tcc/'include',tcc/'include/winapi']:
 assert c.tcc_add_include_path(state,str(inc).encode())==0
assert c.tcc_set_output_type(state,1)==0
files=list((rtc/'mbedtls-2.28.10/library').glob('*.c'))+[host/'dtls_under_test.c',host/'test_dtls_init.c']
for f in files:assert c.tcc_add_file(state,str(Path(f).resolve()).encode())==0,f
result=c.tcc_run(state,0,None)
c.tcc_delete(state)
sys.exit(result)
