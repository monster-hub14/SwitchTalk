import ctypes,sys
from pathlib import Path
c=ctypes.CDLL(str(Path('work/tcc/libtcc.dll').resolve()))
P=ctypes.c_void_p;S=ctypes.c_char_p;I=ctypes.c_int
for name,args,ret in [('tcc_new',[],P),('tcc_set_lib_path',[P,S],None),('tcc_add_include_path',[P,S],I),('tcc_set_output_type',[P,I],I),('tcc_add_file',[P,S],I),('tcc_run',[P,I,P],I),('tcc_delete',[P],None)]:
 f=getattr(c,name);f.argtypes=args;f.restype=ret
c.tcc_define_symbol.argtypes=[P,S,S];c.tcc_define_symbol.restype=None
state=c.tcc_new();c.tcc_define_symbol(state,b'CURL_STATICLIB',b'1');c.tcc_set_lib_path(state,str(Path('work/tcc').resolve()).encode())
for inc in [*sys.argv[1].split(';'),'work/tcc/include','work/tcc/include/winapi']:
 assert c.tcc_add_include_path(state,str(Path(inc).resolve()).encode())==0
assert c.tcc_set_output_type(state,1)==0
for f in sys.argv[2:]:assert c.tcc_add_file(state,str(Path(f).resolve()).encode())==0,f
result=c.tcc_run(state,0,None)
c.tcc_delete(state)
sys.exit(result)
