#ifndef SWITCHTALK_TALK_H
#define SWITCHTALK_TALK_H
#include <stdbool.h>
#define TALK_ROOMS_MAX 96
#define TALK_MESSAGES_MAX 100

typedef struct { char token[128]; char title[192]; int unread; char avatar_user[128],avatar_version[64]; bool custom_avatar; } TalkRoom;
typedef struct { long id; char sender[128]; char body[1024]; char sender_id[128]; } TalkMessage;
typedef struct {
    char server[512], username[128], app_password[256], error[256];
    char account_id[128];
    TalkRoom rooms[TALK_ROOMS_MAX];
    TalkMessage messages[TALK_MESSAGES_MAX];
    int room_count, message_count;
    long poll_cursor;
} TalkClient;
void talk_set_cancel(bool (*cancel)(void*),void *ctx);
void talk_set_poll_wait(unsigned seconds);
bool talk_poll(TalkClient* t,const char* token);
void talk_merge_message(TalkClient* t,const TalkMessage* message);
void talk_init(TalkClient* t);
bool talk_login(TalkClient* t);
bool talk_rooms(TalkClient* t);
bool talk_messages(TalkClient* t,const char* token);
bool talk_send(TalkClient* t,const char* token,const char* message);
void talk_clear_password(TalkClient* t);
#endif
