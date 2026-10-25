#pragma once
#include <stdint.h>
#include <stdbool.h>
typedef uint64_t u64;typedef uint32_t Result;typedef uint32_t Handle;typedef int SmServiceName;
typedef struct {Handle session;int own_handle;} Service;
typedef int AppletHookType;typedef struct {int x;} AppletHookCookie;
#define AppletHookType_OnFocusState 0
#define AppletFocusState_InFocus 1
#define AppletFocusHandlingMode_SuspendHomeSleepNotify 3
#define INVALID_HANDLE 0
#define R_FAILED(x) ((x)!=0)
#define R_SUCCEEDED(x) ((x)==0)
bool serviceIsActive(Service*);void serviceClose(Service*);Result smInitialize(void);void smExit(void);SmServiceName smEncodeName(const char*);int smGetServiceSessionTipc(void);Result smGetServiceOriginal(Handle*,SmServiceName);u64 svcGetSystemTick(void);int appletGetFocusState(void);void appletHook(AppletHookCookie*,void(*)(AppletHookType,void*),void*);void appletUnhook(AppletHookCookie*);void appletSetFocusHandlingMode(int);
Result mock_has(bool*);Result mock_status(void*);Result mock_visible(void*,void*);
#define tipcDispatchInOut(a,b,c,d) mock_has(&(d))
#define serviceDispatchOut(a,b,c) mock_status(&(c))
#define serviceDispatchInOut(a,b,c,d) mock_visible(&(c),&(d))
