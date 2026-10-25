#pragma once
#include <switch.h>
#include <stdatomic.h>
#include "probe.h"
extern atomic_bool alert_stop,alert_test;
void alert_status(ProbeStatus*);
Result alert_start(void);
void alert_join(void);
