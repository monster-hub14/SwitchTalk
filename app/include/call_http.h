#ifndef SWITCHTALK_CALL_HTTP_H
#define SWITCHTALK_CALL_HTTP_H
#include "talk.h"
#include "cJSON.h"
#include <curl/curl.h>
#include <stdbool.h>
bool call_cancelled(void);
void call_set_status(const char *text);
void call_http_init(const TalkClient *client);
void call_http_close(void);
void call_http_use_session(void);
bool call_http_check_room_session(const char *token,const char *expected,char *error,size_t cap);
void call_http_cleanup_mode(void);
cJSON *call_http(const char *method,const char *path,const char *form,char *error,size_t cap);
bool call_ws_open(const char *url,const char *origin,char *error,size_t cap);
bool call_ws_send(cJSON *message);
cJSON *call_ws_next(bool *closed);
void call_ws_close(void);
const char *call_json_string(const cJSON *object,const char *key);
#endif
