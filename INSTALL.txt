SwitchTalk 1.0.1
Author: Monster Review

This patch updates author credits and version labels. Messaging, calls,
notifications and quick replies retain the working 1.0.0 behavior.
The original 1.0.0 golden release is preserved separately.

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

VALIDATION
Author metadata, executable structure and host checks passed. The exact
1.0.1 rebuild has not separately been tested on a Switch.
