#include "talk.h"
#include "tls.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static char url[2048];static int step;
static size_t (*bodyfn)(char*,size_t,size_t,void*),(*headerfn)(char*,size_t,size_t,void*);
static void *bodyctx;
CURL *curl_easy_init(void){return (CURL*)1;}
void curl_easy_cleanup(CURL *c){}
CURLcode curl_easy_setopt(CURL *c,CURLoption option,...){
    va_list a;va_start(a,option);
    switch(option){
        case CURLOPT_URL:strcpy(url,va_arg(a,char*));break;
        case CURLOPT_WRITEFUNCTION:bodyfn=va_arg(a,void*);break;
        case CURLOPT_WRITEDATA:bodyctx=va_arg(a,void*);break;
        case CURLOPT_HEADERFUNCTION:headerfn=va_arg(a,void*);break;
        default:break;
    }
    va_end(a);return CURLE_OK;
}
CURLcode curl_easy_getinfo(CURL *c,CURLINFO info,...){
    va_list a;va_start(a,info);long *code=va_arg(a,long*);
    *code=step==2?201:step==4?304:200;va_end(a);return CURLE_OK;
}
CURLcode curl_easy_perform(CURL *c){
    const char *body=NULL;
    if(step==0){
        assert(strstr(url,"/api/v4/room"));
        body="{\"ocs\":{\"meta\":{\"statuscode\":200},\"data\":[{\"token\":\"room1\",\"displayName\":\"Family\",\"type\":2,\"isCustomAvatar\":true,\"avatarVersion\":\"abc\",\"actorType\":\"users\",\"actorId\":\"me\"},{\"token\":\"room2\",\"name\":\"real-user-id\",\"displayName\":\"A Display Name\",\"type\":1}]}}";
    }else if(step==1){
        assert(strstr(url,"lookIntoFuture=0"));
        body="{\"ocs\":{\"meta\":{\"statuscode\":200},\"data\":[{\"id\":2,\"message\":\"{actor} left the call\",\"messageParameters\":{\"actor\":{\"type\":\"user\",\"id\":\"real-user-id\",\"name\":\"zayd\"}},\"actorType\":\"users\",\"actorId\":\"real-user-id\",\"actorDisplayName\":\"Display Name\"},{\"id\":1,\"message\":\"one\"}]}}";
    }else if(step==2){
        body="{\"ocs\":{\"meta\":{\"statuscode\":201,\"message\":\"OK\"},\"data\":{\"id\":4,\"message\":\"sent\"}}}";
    }else if(step==3){
        assert(strstr(url,"lookIntoFuture=1") && strstr(url,"lastKnownMessageId=2"));
        body="{\"ocs\":{\"meta\":{\"statuscode\":200},\"data\":[{\"id\":3,\"message\":\"Call with {user1} and {user2} ended\",\"messageParameters\":{\"user1\":{\"name\":\"zayd\"},\"user2\":{\"name\":\"Alex\"}}},{\"id\":4,\"message\":\"sent\"}]}}";
        char h[]="X-Chat-Last-Given: 5\r\n";headerfn(h,1,strlen(h),NULL);
    }else if(step==4){assert(strstr(url,"lastKnownMessageId=5"));return CURLE_OK;}
    else {
        body="{\"ocs\":{\"meta\":{\"statuscode\":403,\"message\":\"Forbidden\"},\"data\":[]}}";
    }
    bodyfn((char*)body,1,strlen(body),bodyctx);return CURLE_OK;
}
struct curl_slist *curl_slist_append(struct curl_slist *p,const char *s){return (void*)1;}
void curl_slist_free_all(struct curl_slist *p){}
char *curl_easy_escape(CURL *c,const char *s,int n){return strdup(s);}
void curl_free(void *p){free(p);}
const char *curl_easy_strerror(CURLcode c){return "mock network error";}
CURLcode talk_tls_configure(CURL *c){return CURLE_OK;}
void talk_tls_diagnostic(char *p,size_t n){snprintf(p,n,"mock TLS");}
static TalkClient t;
int main(void){
    talk_init(&t);strcpy(t.server,"https://test.example");
    step=0;assert(talk_rooms(&t));assert(t.room_count==2 && t.rooms[0].custom_avatar && !strcmp(t.rooms[0].avatar_version,"abc") && !strcmp(t.rooms[1].avatar_user,"real-user-id") && !strcmp(t.account_id,"me"));
    puts("API test: history");fflush(stdout);step=1;assert(talk_messages(&t,"room"));assert(t.message_count==2 && t.poll_cursor==2);assert(!strcmp(t.messages[1].sender_id,"real-user-id"));assert(!strcmp(t.messages[1].body,"zayd left the call"));
    puts("API test: send");fflush(stdout);step=2;assert(talk_send(&t,"room","sent"));assert(!t.error[0] && t.poll_cursor==2 && t.message_count==3);
    puts("API test: poll");fflush(stdout);step=3;assert(talk_poll(&t,"room"));assert(t.message_count==4 && t.messages[2].id==3 && t.poll_cursor==5);assert(!strcmp(t.messages[2].body,"Call with zayd and Alex ended"));
    step=4;assert(talk_poll(&t,"room"));assert(t.message_count==4 && t.poll_cursor==5 && !t.error[0]);
    step=5;assert(!talk_poll(&t,"room"));assert(t.message_count==4 && strstr(t.error,"403"));
    puts("API 201 send, forward cursor, concurrent reply, 304 idle, dedup and API error retention: PASS");return 0;
}
