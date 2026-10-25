#pragma once
#include <stdbool.h>
bool notify_write(const char *token,long id,const char *text);
void notify_clear(void);

#include <stdint.h>
void notify_begin(uint64_t instance);
void notify_diagnostics(unsigned *pending,unsigned *real,unsigned *test,unsigned *errors);
