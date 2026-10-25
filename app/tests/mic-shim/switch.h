#ifndef MIC_TEST_SWITCH_H
#define MIC_TEST_SWITCH_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef uint32_t Result;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t s32;
#define R_FAILED(x) ((x)!=0)
typedef struct {int open;} Service;
typedef enum {PcmFormat_Invalid=0,PcmFormat_Int16=2} PcmFormat;
typedef enum {AudioInState_Started=0,AudioInState_Stopped=1} AudioInState;
typedef struct AudioInBuffer {struct AudioInBuffer *next;void *buffer;u64 buffer_size,data_size,data_offset;} AudioInBuffer;
Result audinInitialize(void);
void audinExit(void);
Service *audinGetServiceSession_AudioIn(void);
void serviceClose(Service *s);
Result audinOpenAudioIn(const char*,char*,u32,u32,u32*,u32*,PcmFormat*,AudioInState*);
Result audinStartAudioIn(void);
Result audinStopAudioIn(void);
Result audinListAudioIns(char*,s32,u32*);
Result audinAppendAudioInBuffer(AudioInBuffer*);
Result audinGetReleasedAudioInBuffer(AudioInBuffer**,u32*);
u64 armGetSystemTick(void);
u64 armTicksToNs(u64);
#endif
