#ifndef SWITCH_H
#define SWITCH_H
#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef uint32_t Result;
typedef CRITICAL_SECTION Mutex;
typedef struct { HANDLE handle; void (*entry)(void*);void *arg;} Thread;
#define R_FAILED(r) ((r)!=0)
static void mutexInit(Mutex *m){InitializeCriticalSection(m);}
static void mutexLock(Mutex *m){EnterCriticalSection(m);}
static void mutexUnlock(Mutex *m){LeaveCriticalSection(m);}
static DWORD WINAPI host_entry(void *p){Thread *t=p;t->entry(t->arg);return 0;}
static Result threadCreate(Thread *t,void (*fn)(void*),void *arg,void *stack,size_t size,int priority,int core){
    t->entry=fn;t->arg=arg;return 0;
}
static Result threadStart(Thread *t){t->handle=CreateThread(NULL,0,host_entry,t,0,NULL);return t->handle?0:1;}
static Result threadWaitForExit(Thread *t){WaitForSingleObject(t->handle,INFINITE);return 0;}
static Result threadClose(Thread *t){CloseHandle(t->handle);return 0;}
static void svcSleepThread(int64_t ns){Sleep((DWORD)(ns/1000000));}
#endif
