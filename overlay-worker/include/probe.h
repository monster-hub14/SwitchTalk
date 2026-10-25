#pragma once
#include <stdint.h>
#define PROBE_ID UINT64_C(0x4200000000535755)
#define PROBE_PORT "swtprb"
#define PROBE_VERSION 10u
/* Heartbeats increment in the worker's wait loop, not from elapsed wall time. */
typedef struct {uint32_t version,reserved;uint64_t instance,heartbeats,ticks,pid;uint32_t polls,alerts,failures,mode;uint32_t changes,own,settings,system,blocked,baselines;uint64_t max_id;uint32_t pending,last_real,last_test,write_errors;uint32_t real_alerts,test_alerts;char detail[80];} ProbeStatus;
typedef char StatusMustFitTlsReply[(sizeof(ProbeStatus)+40<=256)?1:-1];
enum {PROBE_STATUS,PROBE_STOP,PROBE_TEST};
