from pathlib import Path
import os,subprocess,sys
r=Path(__file__).resolve().parent;sdk=Path(sys.argv[1]).resolve();out=r/'build';out.mkdir(exist_ok=True);os.environ['DEVKITPRO']=sdk.as_posix()
cc=sdk/('devkitA64/bin/aarch64-none-elf-gcc'+('.exe' if os.name=='nt' else ''))
arch=['-march=armv8-a+crc+crypto','-mtune=cortex-a57','-mtp=soft','-fPIE']
flags=['-g','-O2','-std=gnu11','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-ffunction-sections','-D__SWITCH__',*arch,'-I'+str(r/'include'),'-I'+str(sdk/'libnx/include'),'-I'+str(sdk/'portlibs/switch/include')]
for name in ['worker','controller']:
 objects=[]
 sources=['worker','alert_worker','alert_policy','qr_service','alert_preview','rich_text','notify_file','talk','json','tls','server_url','saved_login'] if name=='worker' else ['controller']
 for unit in sources:
  obj=out/(unit+'.o');objects.append(str(obj))
  subprocess.run([str(cc),*flags,'-c',str(r/'source'/(unit+'.c')),'-o',str(obj)],check=True)
 elf=out/(name+'.elf')
 libs=['-lcurl','-lz','-lmbedcrypto','-lnx','-lm'] if name=='worker' else ['-lnx']
 subprocess.run([str(cc),'-specs='+str(sdk/'libnx/switch.specs'),'-g',*arch,'-Wl,--gc-sections','-Wl,-Map,'+str(out/(name+'.map')),'-Wl,--wrap=sslConnectionDoHandshake',*objects,'-L'+str(sdk/'portlibs/switch/lib'),'-L'+str(sdk/'libnx/lib'),*libs,'-o',str(elf)],check=True)
subprocess.run([sys.executable,str(r/'tools/run_npdmtool_windows.py'),str(Path(sys.argv[2]).resolve()),str(out)],check=True)
from package_sysmodule import pack
pack(out/'worker.elf',out/'main.npdm',out)
subprocess.run([sys.executable,str(r/'package_nro.py'),str(out/'controller.elf'),str(r/'icon.jpg'),str(out/'SwitchTalkAlertsTest.nro')],check=True)
