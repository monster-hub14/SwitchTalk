# SwitchTalk native RTC port

Pinned dependencies:
- libpeer: https://github.com/sepfy/libpeer commit 5b849de378545c31d34759a145413846953e1366 (MIT).
- Mbed TLS 2.28.10: https://github.com/Mbed-TLS/mbedtls (Apache-2.0).
- libsrtp 2.7.0: https://github.com/cisco/libsrtp (BSD).
- App cJSON 1.7.19: https://github.com/DaveGamble/cJSON (MIT).
- Opus 1.3 from devkitPro switch-libopus (BSD), linked separately.

The source directories contain the ported versions used for this release.
The build/librtc-switch.a archive is provided for the Windows app build.
Rebuild it with: python vendor/rtc/build_native.py /path/to/devkitpro
The script uses only Python's standard library and the installed devkitA64 compiler.
It builds all objects each run to incorporate header/config changes.

Port changes: libnx networking and secure randomGet entropy; ECDSA P-256
DTLS certificates; DTLS-SRTP support enabled; data channels disabled;
independent per-peer DTLS timers and incremental handshake; connection state
updates without optional callbacks; bounded ICE/SDP/STUN parsing; address
comparison fixed; authenticated SRTP decryption checked; RTP extension,
CSRC and padding support; standard actpass offers; info logs disabled.

Only IPv4 UDP host/STUN media is enabled in the app. The upstream TURN
allocation code is not a complete relay transport and is not used here.
TURN relay, TCP/TLS media fallback, video and SCTP are outside this release.

Complete licenses accompany the source and install package.

Release 10 fixes DTLS RNG initialization: seed immediately after context
initialization, before server cookie setup; certificate creation reuses the
seeded generator. Cookie context is initialized before any failure path.
Partial SRTP handles are released during teardown. The host regression test
compiles this exact DTLS source with real Mbed TLS implementations.
