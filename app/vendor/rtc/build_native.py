from pathlib import Path
import re,subprocess,concurrent.futures,sys
import os
rtc=Path(__file__).resolve().parent
sdk=Path(sys.argv[1] if len(sys.argv)>1 else os.environ['DEVKITPRO']).resolve()
peer=next(d for d in rtc.glob('libpeer-*') if d.is_dir());mb=rtc/'mbedtls-2.28.10';sr=rtc/'libsrtp-2.7.0';build=rtc/'build';build.mkdir(exist_ok=True)
ext='.exe' if os.name=='nt' else ''
cc=sdk/('devkitA64/bin/aarch64-none-elf-gcc'+ext);ar=sdk/('devkitA64/bin/aarch64-none-elf-ar'+ext)
flags=['-O2','-std=gnu11','-ffunction-sections','-fdata-sections','-march=armv8-a+crc+crypto','-mtune=cortex-a57','-mtp=soft','-fPIE','-DLOG_LEVEL=-1','-D__SWITCH__','-DCONFIG_MBEDTLS_2_X=1','-DCONFIG_USE_USRSCTP=0','-DDISABLE_PEER_SIGNALING=1','-DCONFIG_DTLS_USE_ECDSA=1','-DHAVE_CONFIG_H','-D__unix__','-include','arpa/inet.h']
incs=[sdk/'libnx/include',sdk/'portlibs/switch/include',mb/'include',sr,sr/'include',sr/'crypto/include',peer/'src']
# mbedTLS headers must precede installed package headers.
(build/'endian.h').write_text('#include <sys/endian.h>\n')
incs=[build,mb/'include']+[i for i in incs if i!=mb/'include']
flags += ['-I'+str(p) for p in incs]
srtpfiles=['srtp/srtp.c','crypto/cipher/cipher.c','crypto/cipher/cipher_test_cases.c','crypto/cipher/null_cipher.c','crypto/cipher/aes.c','crypto/cipher/aes_icm.c','crypto/hash/auth.c','crypto/hash/auth_test_cases.c','crypto/hash/null_auth.c','crypto/hash/hmac.c','crypto/hash/sha1.c','crypto/kernel/alloc.c','crypto/kernel/crypto_kernel.c','crypto/kernel/err.c','crypto/kernel/key.c','crypto/math/datatypes.c','crypto/replay/rdb.c','crypto/replay/rdbx.c']
peerskip={'peer_signaling.c','ssl_transport.c'}
sources=list((mb/'library').glob('*.c'))+[build/'entropy.c']+[sr/f for f in srtpfiles]+[f for f in (peer/'src').glob('*.c') if f.name not in peerskip]
def compile(f):
 obj=build/(f.parent.name+'_'+f.stem+'.o')
 p=subprocess.run([str(cc),*flags,'-c',str(f),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:print(f.name+'\n'+p.stderr,flush=True);return None
 return obj
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:objects=list(pool.map(compile,sources))
if any(o is None for o in objects):sys.exit(1)
subprocess.run([str(ar),'rcs',str(build/'librtc-switch.a'),*[str(o) for o in objects]],check=True)
print('RTC archive built:',len(objects),'objects',flush=True)
