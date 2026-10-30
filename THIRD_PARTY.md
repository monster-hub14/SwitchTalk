# Third-party components

Original notices are preserved in the vendored source and component license folders.

- `overlay-worker/vendor/libultrahand`: ppkantorski's libultrahand, with libtesla
  originally by WerWolv; commit `856ddbddd796fc4a59ad2e0bf939c5963e6f9dd2`, the
  revision bundled with Ultrahand 2.5.3. See its LICENSE, SUB_LICENSE and component
  notices, and `overlay-worker/licenses/libultrahand-ATTRIBUTION.txt`.
- `app/vendor/rtc/libpeer-*`: libpeer, MIT; license retained in source and app/licenses.
- `app/vendor/rtc/libsrtp-2.7.0`: libsrtp, BSD-style; license retained.
- `app/vendor/rtc/mbedtls-2.28.10`: Mbed TLS, Apache-2.0 or GPL-2.0-or-later;
  both license choices are retained in source and app/licenses.
- `app/licenses` and `overlay-worker/licenses`: notices for linked libnx, curl,
  zlib, Opus, Mbed TLS, cJSON and font components, as applicable.
- stb image decoding: original notices remain in the source header.
- `app/COPYING.switch-tools`: notice for switch-tools-derived packaging logic.

Build dependencies are listed in BUILD.txt. Nintendo SDK material, Atmosphere,
Ultrahand binaries, user credentials and server configuration are not included.

The updater adapts Monster Review's Switchacade/System Breach verified-slot system.
The native RomFS builder and original switch-tools C sources are in app/tools;
see app/COPYING.switch-tools. The public CA bundle and original notice are retained
under app/romfs/update and app/licenses. Third-party authors/notices are unchanged.
