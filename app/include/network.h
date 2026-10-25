#ifndef SWITCHTALK_NETWORK_H
#define SWITCHTALK_NETWORK_H
#include <switch.h>
#include "talk.h"
typedef enum { NET_LOGIN,NET_ROOMS,NET_HISTORY,NET_SEND,NET_POLL,NET_AUTO_ROOMS } NetAction;
Result network_start(void);
void network_stop(void);
void network_pause(bool paused);
bool network_submit(NetAction action,const TalkClient *client,const char *token,const char *message);
bool network_take(TalkClient *client,NetAction *action,bool *ok,char *notice,size_t cap);
void network_watch(const char *token);
void network_signout(void);
#endif
