SwitchTalk 1.0.0 - Golden Release

This archive freezes the app and overlay/worker behavior confirmed working on
your Switch: full app Release 17 and Quick Reply Release 2. Only release labels,
visible names, HTTP user-agent version text and application metadata changed.
All three visible components now show 1.0.0:
  SwitchTalk - full application
  SwitchTalk Overlay - Ultrahand conversations and keyboard replies
  SwitchTalk Worker - background message worker controller
The resident service executable is included with the worker controller.

REQUIREMENTS
Atmosphere homebrew on an original Switch, OLED or Lite; not Switch 2.
The tested overlay setup uses Ultrahand 2.5.3. Keep your existing Ultrahand and
sys-tune installation. They are separate dependencies, not bundled in this ZIP.
Nextcloud Talk account and a reachable HTTPS server with a trusted certificate.
The Switch date and time must be correct for HTTPS certificate verification.

INSTALL OR UPGRADE
Power off, open the SD card, and copy the atmosphere and switch folders from
this ZIP to the SD root. Merge folders and replace the included files. Reboot.
No account or credential file is included; existing saved login is retained.
Files:
  switch/SwitchTalk/SwitchTalk.nro
  switch/.overlays/SwitchTalk.ovl
  switch/SwitchTalkAlertsTest/SwitchTalkAlertsTest.nro
  atmosphere/contents/4200000000535755/exefs.nsp
The worker controller keeps its previous disk path so an upgrade replaces the
old launcher. Its homebrew-menu name is now SwitchTalk Worker, version 1.0.0.
No boot2.flag or retired background voice module is installed.

FIRST USE
1. Open SwitchTalk, sign in and confirm chats load. Your login is saved.
2. Open SwitchTalk Worker, press A and wait for Monitoring (X updates status).
   R requests a test notification. PLUS closes the controller but keeps the
   worker running. Y explicitly stops it.
3. Launch your game. Open Ultrahand normally and select SwitchTalk Overlay.
   A can start/connect the worker if needed. Select a conversation to read it.
Start the worker again after reboot; automatic boot startup is not enabled.

OVERLAY CONTROLS
Conversation list: Up/Down selects, Left/Right changes pages, A opens a room.
Chat: Up/Down or dragging scrolls; Y refreshes; A or X opens the keyboard.
Keyboard: D-pad selects, A types, X deletes, Y inserts space, L changes case,
R changes letters/symbols. Touch typing works. Select Send to send the reply.
B goes back one level: keyboard -> chat -> conversations -> Ultrahand.
PLUS hides the overlay and returns to your game.
Drafts remain in memory when hiding/reopening, but are lost on unload/reboot.
The initial keyboard supports English letters, numbers and punctuation,
up to 512 bytes per reply, with editing at the end of the text.
Unconfirmed sends keep the draft; B then Y lets you review chat before retrying.
No automatic resend is performed.

MESSAGES AND CALLS
Direct and group notifications use All incoming, checked about every 8 seconds.
The room actually visible in the app or overlay suppresses its own popups.
Other rooms still notify. Hiding the overlay or leaving the app restores alerts,
even if a chat remains selected. Visibility signals expire after six seconds
without renewal. Messages suppressed while viewing are not replayed afterward.
Foreground calls and the mic test stay in the full application. Background
voice during gameplay is not part of this release.

GOLDEN RELEASE RECORD
Build, host tests, binary structure and AMS 1.10+ runtime checks passed.
Source comparison confirms only the listed text/metadata changes from the
user-confirmed working versions. The rebuilt 1.0.0 binaries have not separately
been tested on hardware. SHA256.txt identifies every shipped payload file.
The external SHA256 file also identifies the install, source and debug ZIPs.
This archive is frozen: future changes must use a new release version.
