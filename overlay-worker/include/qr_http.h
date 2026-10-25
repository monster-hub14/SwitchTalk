#pragma once
#include "quick_reply.h"
#include "talk.h"
bool qr_http_history(TalkClient*,const char*,QRMessage*,int*);
bool qr_http_send(TalkClient*,const char*,const char*,QRMessage*);
