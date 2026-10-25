#ifndef SWITCHTALK_CALL_VERSION_H
#define SWITCHTALK_CALL_VERSION_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define CALL_VERSION 14u
typedef struct {uint32_t version;char server[512],username[128],password[256],room[128],title[192];} CallRequest;
typedef struct {uint32_t version,active,muted,sent,received,linked;char title[192],status[240];} CallState;
bool call_validate_request(const CallRequest *start);
void call_erase(void *memory,size_t size);
#endif
