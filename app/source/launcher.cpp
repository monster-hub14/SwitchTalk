#include "update-core.hpp"
#include <cstdio>
#include <switch.h>
int main() {
  consoleInit(nullptr);
  update::Slots slots;
  bool recovered = false;
  char slot = slots.bootChoice(recovered);
  if (slot && envHasNextLoad()) {
    auto path = slots.file(slot);
    if (update::writeSafe(slots.root + "/booting.txt", std::string(1, slot) + "\n") &&
        R_SUCCEEDED(envSetNextLoad(path.c_str(), path.c_str()))) {
      consoleExit(nullptr);
      return 0;
    }
  }
  printf("SWITCHTALK - RECOVERY\n\n");
  if (recovered)
    printf("A previous launch failed. Backup selected.\n");
  printf(slot ? "Your loader could not start the app.\nOpen app-a.nro / app-b.nro "
                "from this folder,\nor launch through the Homebrew Menu.\n"
              : "No verified app version was found.\nReinstall the complete "
                "SwitchTalk ZIP.\n");
  printf("\nPLUS returns to the Homebrew Menu.\n");
  padConfigureInput(1, HidNpadStyleSet_NpadStandard);
  PadState pad;
  padInitializeDefault(&pad);
  while (appletMainLoop()) {
    padUpdate(&pad);
    if (padGetButtonsDown(&pad) & HidNpadButton_Plus)
      break;
    consoleUpdate(nullptr);
  }
  consoleExit(nullptr);
  return 1;
}
