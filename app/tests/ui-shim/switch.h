#ifndef SWITCHTALK_TEST_UI_SWITCH_H
#define SWITCHTALK_TEST_UI_SWITCH_H
#pragma once
#include <windows.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
typedef CRITICAL_SECTION Mutex;
static void mutexInit(Mutex *m){InitializeCriticalSection(m);}
static void mutexLock(Mutex *m){EnterCriticalSection(m);}
static void mutexUnlock(Mutex *m){LeaveCriticalSection(m);}
typedef uint32_t Result;typedef uint32_t Handle;typedef uint64_t u64;typedef struct {int unused;} Service;typedef struct {int unused;} PadState;
#define R_FAILED(x) ((x)!=0)
#define R_SUCCEEDED(x) ((x)==0)
#define MAKERESULT(m,d) ((m)|((d)<<9))
#define HidNpadButton_X 1u
#define HidNpadButton_Y 2u
#define HidNpadButton_B 4u
#define HidNpadButton_Plus 8u
void padInitializeDefault(PadState *s);
void padUpdate(PadState *s);
u64 padGetButtonsDown(PadState *s);
bool appletMainLoop(void);
void consoleUpdate(void *p);
void svcSleepThread(int64_t ns);
int ui_test_printf(const char *fmt,...);
#define printf ui_test_printf


#endif
