#pragma once
#include <switch.h>
#include "call_state.h"
void call_engine_init(void);
Result call_engine_start(const CallRequest *start);
void call_engine_status(CallState *out);
void call_engine_mute(void);
void call_engine_stop(void);

void call_engine_join(void);
