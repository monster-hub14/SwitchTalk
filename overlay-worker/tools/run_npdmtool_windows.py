from pathlib import Path
import ctypes,sys
r=Path(__file__).resolve().parent.parent;t=Path(sys.argv[1]).resolve();out=Path(sys.argv[2]).resolve();out.mkdir(parents=True,exist_ok=True)
c=ctypes.CDLL(str(t/'libtcc.dll'));P=ctypes.c_void_p;S=ctypes.c_char_p;I=ctypes.c_int
for n,args,ret in [('tcc_new',[],P),('tcc_set_lib_path',[P,S],None),('tcc_add_include_path',[P,S],I),('tcc_set_output_type',[P,I],I),('tcc_add_file',[P,S],I),('tcc_run',[P,I,P],I),('tcc_delete',[P],None)]:
 f=getattr(c,n);f.argtypes=args;f.restype=ret
state=c.tcc_new();c.tcc_set_lib_path(state,str(t).encode())
for inc in [r/'include',t/'include',t/'include/winapi']:assert c.tcc_add_include_path(state,str(inc.resolve()).encode())==0
assert c.tcc_set_output_type(state,1)==0
for f in [r/'source/cJSON.c',r/'tools/host_compat.c',r/'tools/npdmtool.c']:assert c.tcc_add_file(state,str(f.resolve()).encode())==0
args=[b'npdmtool',str((r/'sysmodule/SwitchTalk.json').resolve()).encode(),str(out/'main.npdm').encode()];argv=(S*len(args))(*args)
result=c.tcc_run(state,len(args),argv);c.tcc_delete(state);sys.exit(result)
