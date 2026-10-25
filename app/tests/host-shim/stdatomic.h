#ifndef STDATOMIC_H
#define STDATOMIC_H
#include <windows.h>
typedef volatile LONG atomic_bool;
#define atomic_init(p,v) InterlockedExchange((p),(v))
#define atomic_store(p,v) InterlockedExchange((p),(v))
#define atomic_load(p) InterlockedCompareExchange((p),0,0)
#endif
