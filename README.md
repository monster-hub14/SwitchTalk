# SwitchTalk

**Author: Monster Review**

Nextcloud Talk messaging and foreground voice calls for Nintendo Switch homebrew,
with in-game message notifications and quick replies through Ultrahand.

<a href="https://www.buymeacoffee.com/monsterreview"><img src="https://www.buymeacoffee.com/assets/img/custom_images/yellow_img.png" alt="Buy Me a Coffee" width="200"></a>

**Version 1.1.0** adds optional GitHub automatic updates, enabled by default.
The frozen 1.0.0 golden release remains available.

## Before you start

SwitchTalk is a client for an existing Nextcloud Talk service, not a standalone communication app. Before you can use it, you need access to a working Nextcloud server with Nextcloud Talk already installed and configured, and an account on that server. You can use your own server or one provided by someone else. SwitchTalk does not host a server or create a communication service for you; without a working Nextcloud and Nextcloud Talk setup, it will not work.

## Why are there two NRO files?

**Always open SwitchTalk from Homebrew Menu using `SwitchTalk.nro`. Keep both files.**

| File | Purpose | What you should do |
| --- | --- | --- |
| `SwitchTalk.nro` | Stable launcher: verifies and starts the selected application version. | Launch this normally. |
| `app-a.nro` | Actual SwitchTalk application in update slot A. | Keep it in the same folder; the launcher opens it automatically. |
| `app-b.nro` | Alternate application slot, created by a future update. | Keep it when it appears; it holds the other verified version for updating/recovery. |

Install the entire `switch/SwitchTalk/` folder. Do not delete `app-a.nro`, rename
these files, move them apart, or replace `SwitchTalk.nro` with an application-slot
file or the separate `SwitchTalkApp.nro` release asset.

Updates download into the slot that is not running, then the launcher switches
to the verified version. The previous verified slot remains available for recovery.
You do not need to choose between A and B or open the slot files yourself during
normal use. The supplied `.txt` files are also part of the updater; keep them.

## Automatic updates are optional - ON by default

Install the **complete 1.1.0 ZIP once** to upgrade from 1.0.x. Copy both `switch`
and `atmosphere` folders, merge/replace, and reboot. Saved login is retained.
The new `SwitchTalk.nro` is a stable launcher. Do not replace it with the separate
`SwitchTalkApp.nro` update asset. Keep `/switch/SwitchTalk/` in place.

**To turn automatic updates off:**

1. Open the full SwitchTalk app, on sign-in or Conversations.
2. Press **ZL**, or tap **ZL Updates** in the header.
3. Press **X** until it shows **AUTOMATIC UPDATES: OFF**.
4. Press **B** to return.

Your choice stays saved after app/console restarts. Disabling requests cancellation
of a running automatic transfer and prevents automatic installation. Chat, calls
and notifications continue normally. **B/Later only postpones this session; use X
to disable persistently.**

With updates off, **A** in Updates checks/downloads manually; press **A** again
when verified to install. Manual checking does not turn automatic updates on.
**X** turns them back on for the next app launch. **Y** restores an older verified
slot when available and disables automatic updates.

When enabled, the FULL app checks stable GitHub releases when opened. Downloads
use HTTPS plus size, SHA-256, title/version and slot checks. Installation waits
outside chats/calls and active user operations for a five-second restart countdown.
**B** postpones. The worker/overlay alone do not check for updates during gameplay.
The payload includes matching overlay, worker and controller files; they synchronize
on app startup, stopping a running worker before replacement and restarting it
where possible. Return to Ultrahand's menu before reopening the updated overlay.

The executing slot is never overwritten. A stable launcher can restore a verified
backup after corruption or failure before the first app frame. Interrupted companion
installation is repaired when reopening the selected verified app. Reinstall the
full ZIP if neither slot works. Reopen through hbmenu if your loader cannot restart.
The stable launcher's hbmenu icon/version is not automatically replaced; the version
inside SwitchTalk identifies the running app. See [UPDATES.md](UPDATES.md).

This exact SwitchTalk updater/relaunch needs physical Switch testing. Host tests
cover transfer failures, validation, saved settings, rollback, idle-only activation
and companion repair; native app/launcher/RomFS checks also pass.

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
- Access to an existing Nextcloud server with **Nextcloud Talk already installed and configured**,
  an account on that server, and a reachable HTTPS address with a trusted certificate.
  Set the Switch date and time correctly for certificate verification.
- A supported microphone for calls: 3.5 mm headset or compatible USB input.
  Bluetooth microphone input is not supported.

## Download and install

Download **SwitchTalk-1.1.0.zip** from [Releases](https://github.com/monster-hub14/SwitchTalk/releases/latest).
The ZIP contains the compiled application, overlay, resident worker and its controller.
Power off the Switch, extract the `switch` and `atmosphere` folders to the SD root,
merge folders, replace existing SwitchTalk files, and reboot.

| Component | SD path |
| --- | --- |
| Stable launcher | `switch/SwitchTalk/SwitchTalk.nro` |
| Verified application slot | `switch/SwitchTalk/app-a.nro` |
| Ultrahand overlay | `switch/.overlays/SwitchTalk.ovl` |
| Worker controller | `switch/SwitchTalkAlertsTest/SwitchTalkAlertsTest.nro` |
| Resident worker | `atmosphere/contents/4200000000535755/exefs.nsp` |

The worker controller keeps its existing disk path for upgrades. Its displayed name
is **SwitchTalk Worker**, version **1.1.0**. Existing saved login is retained.
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

