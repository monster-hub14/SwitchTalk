#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef uint32_t Result;typedef uint32_t Handle;typedef uint64_t u64;typedef struct {Handle session;unsigned own_handle;} Service;typedef struct {int unused;} PadState;
#define R_FAILED(x) ((x)!=0)
#define R_SUCCEEDED(x) ((x)==0)
#define MAKERESULT(m,d) ((m)|((d)<<9))
#define HidNpadButton_L 64u
#define HidNpadButton_R 32u
#define HidNpadButton_A 1u
#define HidNpadButton_X 2u
#define HidNpadButton_Y 4u
#define HidNpadButton_B 8u
#define HidNpadButton_Plus 16u
#define HidNpadStyleSet_NpadStandard 0
#define SfBufferAttr_Out 1u
#define SfBufferAttr_HipcMapAlias 2u
typedef struct {unsigned buffer_attrs[8];struct {const void *ptr;size_t size;} buffers[8];} MockDispatch;
Result mock_dispatch(Service*,unsigned,MockDispatch);
#define serviceDispatch(s,c,...) mock_dispatch((s),(c),(MockDispatch){__VA_ARGS__})
Result svcConnectToNamedPort(Handle*,const char*);void serviceCreate(Service*,Handle);void serviceClose(Service*);
void *consoleInit(void*);void consoleExit(void*);void consoleUpdate(void*);
void padConfigureInput(unsigned,unsigned);void padInitializeDefault(PadState*);void padUpdate(PadState*);u64 padGetButtonsDown(PadState*);
bool appletMainLoop(void);void svcSleepThread(int64_t);
typedef struct {uint64_t program_id;unsigned storageID;} NcmProgramLocation;
#define NcmStorageId_None 0
Result pmshellInitialize(void);Result pmshellLaunchProgram(uint32_t,const NcmProgramLocation*,uint64_t*);void pmshellExit(void);
int probe_test_printf(const char*,...);
#define printf probe_test_printf

#define KERNELRESULT(x) 0xf201u
#define serviceDispatchOut(s,c,o) mock_dispatch((s),(c),(MockDispatch){.buffers={{&(o),sizeof(o)}}})

typedef struct {char name[8];} SmServiceName;
SmServiceName smEncodeName(const char*);
Service *smGetServiceSessionTipc(void);
Result smGetServiceOriginal(Handle*,SmServiceName);
Result mock_has_service(SmServiceName,bool*);
#define tipcDispatchInOut(s,c,i,o) mock_has_service((i),&(o))
