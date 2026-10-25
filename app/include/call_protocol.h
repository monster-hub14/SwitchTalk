#ifndef CALL_PROTOCOL_H
#define CALL_PROTOCOL_H
#include <stddef.h>
char *call_sdp_answer(const char *local,const char *remote,char *mid,size_t cap);
#endif
