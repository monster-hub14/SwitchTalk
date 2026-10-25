# SwitchTalk

Nextcloud Talk messaging and foreground voice calls for Nintendo Switch homebrew,
with in-game message notifications and quick replies through Ultrahand.

**Version 1.0.0** is the frozen golden release, built from versions tested by the
project owner on a Switch. Names and version labels were unified for distribution.

## Features

- Direct and group messaging, saved sign-in, avatars, and rich-message names.
- Foreground conference calls and a microphone-input test in the full app.
- Background direct/group message notifications with sender and message previews.
- An Ultrahand conversation viewer and controller/touch reply keyboard.
- Suppression of notifications for the room currently visible, with alerts resuming
  when you hide the overlay or leave the app.

Background voice during gameplay is not included.

## Requirements

- An original Nintendo Switch, Switch OLED, or Switch Lite running Atmosphere homebrew.
  Switch 2 is not supported.
- Ultrahand 2.5.3 for notifications and the overlay. Atmosphere, Ultrahand, and sys-tune
  are separate projects and are not bundled.
- A Nextcloud Talk account on a reachable HTTPS server with a trusted certificate.
  Set the Switch date and time correctly for certificate verification.
- A supported microphone for calls: 3.5 mm headset or compatible USB input.
  Bluetooth microphone input is not supported.

## Download and install

Download **SwitchTalk-1.0.0-Golden.zip** from [Releases](https://github.com/monster-hub14/SwitchTalk/releases/latest).
The ZIP contains the compiled application, overlay, resident worker and its controller.
Power off the Switch, extract the `switch` and `atmosphere` folders to the SD root,
merge folders, replace existing SwitchTalk files, and reboot.

| Component | SD path |
| --- | --- |
| Full application | `switch/SwitchTalk/SwitchTalk.nro` |
| Ultrahand overlay | `switch/.overlays/SwitchTalk.ovl` |
| Worker controller | `switch/SwitchTalkAlertsTest/SwitchTalkAlertsTest.nro` |
| Resident worker | `atmosphere/contents/4200000000535755/exefs.nsp` |

The worker controller keeps its existing disk path for upgrades. Its displayed name
is **SwitchTalk Worker**, version **1.0.0**. Existing saved login is retained.
No account data or boot-startup flag is shipped.

1. Open SwitchTalk, sign in and confirm your conversations load.
2. Open SwitchTalk Worker, press **A**, and wait for Monitoring; **X** updates status.
   **R** requests a test popup. **+** exits while leaving the worker running.
3. Launch a game, open Ultrahand normally, and select **SwitchTalk Overlay**.
   **A** can start/connect the worker when needed.

Start the worker again after reboot; automatic boot startup is not enabled.
See [INSTALL.txt](INSTALL.txt) for complete controls and operational details.

## Quick replies

Open a conversation and press **A** or **X** to compose. Use D-pad + A or touch
to type, **X** to delete, **Y** for space, **L** for case and **R** for symbols.
Select **Send** to send. **B** returns one level: keyboard → chat → conversations
→ Ultrahand. **+** hides the overlay and returns to your game.

The initial keyboard supports English letters, numbers and punctuation, with a
512-byte reply limit. Drafts remain in memory while hiding/reopening; they are
not saved across overlay unloads or reboot. Unconfirmed sends keep the draft;
review the chat before retrying. There is no automatic resend.

## Build and validation

See [BUILD.txt](BUILD.txt). The release uses devkitA64 r28 / GCC 15.1.0 and libnx
4.12.0. The pinned libultrahand revision is vendored with source and notices.

The source is organized as `app/` and `overlay-worker/`. Host tests cover messaging,
API handling, calls, avatars, notifications, visibility, drafts, bounded IPC and
keyboard editing. Binary validation checks NRO/NSO/NPDM layout and the runtime
extension required by Atmosphere 1.10+.

Golden ZIP checksums and a release manifest are attached to the release. The golden
artifacts are immutable; subsequent changes must receive a new version number.
The 1.0.0 rebuild passed host/build checks; the exact relabeled binaries have not
separately undergone a hardware test. Their functional source matches the working
versions confirmed by the project owner.

## License and credits

SwitchTalk is distributed under **GPL-2.0-or-later**. Dependencies retain their own
licenses and notices. See [LICENSE](LICENSE) and [THIRD_PARTY.md](THIRD_PARTY.md).

Thanks to the libnx/devkitPro, Nextcloud Talk, Ultrahand/libultrahand, libtesla,
libpeer, libsrtp, Mbed TLS, curl, Opus, zlib, cJSON, and stb projects.
This is an independent homebrew project, unaffiliated with Nintendo or Nextcloud.

