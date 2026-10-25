from pathlib import Path
import ctypes,sys
r=Path(__file__).resolve().parent;tcc=Path(sys.argv[1]).resolve();out=r/'build';out.mkdir(exist_ok=True)
c=ctypes.CDLL(str(tcc/'libtcc.dll'));P=ctypes.c_void_p;S=ctypes.c_char_p;I=ctypes.c_int
for n,args,ret in [('tcc_new',[],P),('tcc_set_lib_path',[P,S],None),('tcc_add_include_path',[P,S],I),('tcc_define_symbol',[P,S,S],None),('tcc_set_output_type',[P,I],I),('tcc_add_file',[P,S],I),('tcc_run',[P,I,P],I),('tcc_delete',[P],None)]:
 f=getattr(c,n);f.argtypes=args;f.restype=ret
state=c.tcc_new();c.tcc_set_lib_path(state,str(tcc).encode());c.tcc_define_symbol(state,b'PROBE_HOST',b'1');c.tcc_define_symbol(state,b'BASELINE',('"'+(out/'test-baseline.bin').as_posix()+'"').encode())
for inc in [r/'tests',r/'include',tcc/'include',tcc/'include/winapi']:assert c.tcc_add_include_path(state,str(inc).encode())==0
assert c.tcc_set_output_type(state,1)==0;assert c.tcc_add_file(state,str(r/'tests/test_controller.c').encode())==0
result=c.tcc_run(state,0,None);c.tcc_delete(state);assert not result
print('Residency controller: X never launches, A records baseline, close keeps worker, reopen checks same instance/counter, restart detected, explicit stop, launch rejection handled: PASS')
