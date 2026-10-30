# SwitchTalk updates

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

