#pragma once
#include <stdbool.h>
#include <stdint.h>
#define ALERT_ROOMS 96
typedef struct {char token[128],title[192],actor[128],sender[128],body[1024];long id;int unread,level,type;bool system,silent;} AlertRoom;
typedef struct {char token[128];long id;bool active;} AlertCursor;
typedef struct {unsigned changes,own,settings,system,blocked,baselines,rooms,ids;long max_id;char last[112];} AlertStats;
typedef struct {AlertCursor rooms[ALERT_ROOMS];bool initialized;AlertStats stats;} AlertPolicy;
typedef bool (*AlertEmit)(const AlertRoom*,void*);
void alert_scan(AlertPolicy*,const AlertRoom*,int,const char*,const char*,AlertEmit,void*);

#include "talk.h"
bool alert_fetch(TalkClient*,AlertRoom*,int*);
