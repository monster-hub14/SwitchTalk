#pragma once
// Desktop adapter for syntax checking and rendering. Not a Switch SDK
// replacement.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using u32 = uint32_t;
using u64 = uint64_t;
using Result = int;
#define R_SUCCEEDED(x) ((x) == 0)
#define R_FAILED(x) ((x) != 0)
struct Framebuffer {};
constexpr int PIXEL_FORMAT_RGBA_8888 = 1;
inline void *nwindowGetDefault() { return nullptr; }
inline Result framebufferCreate(Framebuffer *, void *, int, int, int, int) { return 0; }
inline Result framebufferMakeLinear(Framebuffer *) { return 0; }
inline void framebufferClose(Framebuffer *) {}
inline u32 pixels[1280 * 720];
inline void *framebufferBegin(Framebuffer *, u32 *stride) {
  *stride = 1280 * 4;
  return pixels;
}
inline void framebufferEnd(Framebuffer *) {
#if defined(SWITCHACADE_TEST) && !defined(SWITCHACADE_CAPTURE)
  return; // Flow tests still render but do not write a screenshot every frame.
#endif
  FILE *f = fopen("work/menu.ppm", "wb");
  if (f) {
    fprintf(f, "P6\n1280 720\n255\n");
    for (u32 p : pixels) {
      unsigned char b[3] = {static_cast<unsigned char>(p), static_cast<unsigned char>(p >> 8),
                            static_cast<unsigned char>(p >> 16)};
      fwrite(b, 1, 3, f);
    }
    fclose(f);
  }
}
struct PadState {};
constexpr int HidNpadStyleSet_NpadStandard = 1;
constexpr u64 HidNpadButton_Plus = 1, HidNpadButton_B = 2, HidNpadButton_A = 4,
              HidNpadButton_Left = 8, HidNpadButton_Right = 16, HidNpadButton_L = 32,
              HidNpadButton_R = 64, HidNpadButton_X = 128, HidNpadButton_Y = 256,
              HidNpadButton_Down = 512, HidNpadButton_Up = 1024, HidNpadButton_ZR = 2048, HidNpadButton_ZL = 4096, HidNpadButton_Minus = 8192;
inline void padConfigureInput(int, int) {}
inline void padInitializeDefault(PadState *) {}
inline void padUpdate(PadState *) {}
inline int previewFrame = 0;
inline int testFramesRemaining = 0;
inline int previewGame() {
  const char *v = std::getenv("SWITCHACADE_PREVIEW_GAME");
  if (v)
    return std::atoi(v);
  return std::getenv("SWITCHACADE_PREVIEW_HANGMAN") ? 1 : -1;
}
inline u64 padGetButtonsDown(PadState *) {
  int g = previewGame();
  if (g >= 0)
    return previewFrame <= g ? HidNpadButton_Right : HidNpadButton_A;
  return 0;
}
inline bool appletMainLoop() {
#ifdef SWITCHACADE_TEST
  return testFramesRemaining-- > 0;
#endif
  int g = previewGame();
  return previewFrame++ < (g >= 0 ? g + 1 : 1);
}
inline Result socketInitializeDefault() { return 0; }
inline void socketExit() {}
constexpr int NifmServiceType_User = 0;
inline Result nifmInitialize(int) { return 0; }
inline void nifmExit() {}
inline void randomGet(void *p, size_t n) { memset(p, 42, n); }
constexpr int AccountServiceType_Application = 0;
struct AccountUid {};
struct AccountProfile {};
struct AccountProfileBase {
  char nickname[32];
};
inline Result accountInitialize(int) { return 1; }
inline void accountExit() {}
inline Result accountGetPreselectedUser(AccountUid *) { return 1; }
inline Result accountGetLastOpenedUser(AccountUid *) { return 1; }
inline bool accountUidIsValid(AccountUid *) { return false; }
inline Result accountGetProfile(AccountProfile *, AccountUid) { return 1; }
inline Result accountProfileGet(AccountProfile *, void *, AccountProfileBase *) { return 1; }
inline void accountProfileClose(AccountProfile *) {}
struct SwkbdConfig {};
inline Result swkbdCreate(SwkbdConfig *, int) { return 1; }
inline void swkbdConfigMakePresetDefault(SwkbdConfig *) {}
inline void swkbdConfigSetHeaderText(SwkbdConfig *, const char *) {}
inline void swkbdConfigSetInitialText(SwkbdConfig *, const char *) {}
inline void swkbdConfigSetStringLenMax(SwkbdConfig *, int) {}
inline Result swkbdShow(SwkbdConfig *, char *, size_t) { return 1; }
inline void swkbdClose(SwkbdConfig *) {}

inline Result nifmGetCurrentIpConfigInfo(u32 *ip, u32 *mask, u32 *, u32 *, u32 *) {
  *ip = 0;
  *mask = 0;
  return 1;
}

struct FakeStick {
  int x = 0, y = 0;
};
inline FakeStick testSticks[2];
inline FakeStick padGetStickPos(PadState *, int i) { return testSticks[i]; }
inline u64 testHeld = 0;
inline u64 padGetButtons(PadState *) { return testHeld; }

struct HidTouchState {u32 x=0,y=0;};
struct HidTouchScreenState {int count=0;HidTouchState touches[16];};
inline HidTouchScreenState testTouch{};
inline void hidInitializeTouchScreen(){}
inline size_t hidGetTouchScreenStates(HidTouchScreenState*states,size_t count){if(count)*states=testTouch;return count?1:0;}
