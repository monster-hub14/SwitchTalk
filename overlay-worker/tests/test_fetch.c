#include "alert_policy.h"
#include "qr_http.h"
#include "talk.h"
#include "tls.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static char post[2048];static char url[2048];static int step;
static size_t (*bodyfn)(char*,size_t,size_t,void*),(*headerfn)(char*,size_t,size_t,void*);
static void *bodyctx;
CURL *curl_easy_init(void){post[0]=0;return (CURL*)1;}
void curl_easy_cleanup(CURL *c){}
CURLcode curl_easy_setopt(CURL *c,CURLoption option,...){
    va_list a;va_start(a,option);
    switch(option){
        case CURLOPT_POSTFIELDS:strcpy(post,va_arg(a,char*));break;
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
    *code=step==2?401:step==9?304:step==10?403:step==11||step==12?201:step==13?403:step==15?0:200;va_end(a);return CURLE_OK;
}
CURLcode curl_easy_perform(CURL *c){
 if(step>=11){
  assert(strstr(url,"/api/v1/chat/newer?format=json") && !strcmp(post,"message=Hello%20%26%20%2B%25"));
  if(step==15)return CURLE_OPERATION_TIMEDOUT;
  const char *sent=step==12?"not JSON":"{\"ocs\":{\"meta\":{\"statuscode\":201},\"data\":{\"id\":55,\"message\":\"Hello & +%\"}}}";
  bodyfn((char*)sent,1,strlen(sent),bodyctx);return CURLE_OK;
 }
 if(step==3)return CURLE_PEER_FAILED_VERIFICATION;
 if(step<8)assert(strstr(url,"/api/v4/room?") && strstr(url,"noStatusUpdate=1"));if(step<8)assert(!strstr(url,"/chat/"));
 const char *body="{\"ocs\": {\"meta\": {\"statuscode\": 200}, \"data\": [{\"token\": \"room\", \"displayName\": \"Family\", \"type\": 2, \"notificationLevel\": 1, \"unreadMessages\": 4, \"actorType\": \"users\", \"actorId\": \"my-id\", \"lastMessage\": {\"id\": 52, \"actorType\": \"users\", \"actorId\": \"other\", \"message\": \"Hello {user1}, ready to play?\", \"systemMessage\": \"\", \"messageType\": \"comment\", \"actorDisplayName\": \"Alice\", \"messageParameters\": {\"user1\": {\"type\": \"user\", \"id\": \"me\", \"name\": \"Zayd\"}}}}]}}";
 if(step==4)body="{\"ocs\": {\"meta\": {\"statuscode\": 200}, \"data\": [{\"token\": \"room\", \"displayName\": \"Family\", \"type\": 2, \"notificationLevel\": 1, \"unreadMessages\": 4, \"actorType\": \"users\", \"actorId\": \"my-id\", \"lastMessage\": {\"id\": 52, \"actorType\": \"users\", \"actorId\": \"other\", \"message\": \"secret body never exported\", \"systemMessage\": false, \"messageType\": \"comment\", \"silent\": false}}]}}";
 if(step==5)body="{\"ocs\": {\"meta\": {\"statuscode\": 200}, \"data\": [{\"token\": \"room\", \"displayName\": \"Family\", \"type\": 2, \"notificationLevel\": 1, \"unreadMessages\": 4, \"actorType\": \"users\", \"actorId\": \"my-id\", \"lastMessage\": {\"id\": 52, \"actorType\": \"users\", \"actorId\": \"other\", \"message\": \"secret body never exported\", \"systemMessage\": null, \"messageType\": \"comment\", \"silent\": false}}]}}";
 if(step==6)body="{\"ocs\": {\"meta\": {\"statuscode\": 200}, \"data\": [{\"token\": \"room\", \"displayName\": \"Family\", \"type\": 2, \"notificationLevel\": 1, \"unreadMessages\": 4, \"actorType\": \"users\", \"actorId\": \"my-id\", \"lastMessage\": {\"id\": 52, \"actorType\": \"users\", \"actorId\": \"other\", \"message\": \"secret body never exported\", \"systemMessage\": \"call_ended\", \"messageType\": \"comment\", \"silent\": false}}]}}";
 if(step==7)body="{\"ocs\": {\"meta\": {\"statuscode\": 200}, \"data\": [{\"token\": \"room\", \"displayName\": \"Family\", \"type\": 2, \"notificationLevel\": 1, \"unreadMessages\": 4, \"actorType\": \"users\", \"actorId\": \"my-id\", \"lastMessage\": {\"id\": 52, \"actorType\": \"users\", \"actorId\": \"other\", \"message\": \"secret body never exported\", \"systemMessage\": \"\", \"messageType\": \"comment\", \"silent\": true}}]}}";
 if(step>=8){assert(strstr(url,"/api/v1/chat/newer?") && strstr(url,"limit=10") && strstr(url,"setReadMarker=0") && strstr(url,"markNotificationsAsRead=0") && strstr(url,"noStatusUpdate=1"));body="{\"ocs\": {\"meta\": {\"statuscode\": 200}, \"data\": [{\"id\": 2, \"actorDisplayName\": \"Bob\", \"message\": \"Hello {user1}\", \"messageParameters\": {\"user1\": {\"type\": \"user\", \"name\": \"Alice\"}}}, {\"id\": 1, \"actorDisplayName\": \"Alice\", \"message\": \"First\"}]}}";if(step==9)return CURLE_OK;}
 if(step==1)body="not JSON";
 bodyfn((char*)body,1,strlen(body),bodyctx);return CURLE_OK;
}
struct curl_slist *curl_slist_append(struct curl_slist *p,const char *s){return (void*)1;}
void curl_slist_free_all(struct curl_slist *p){}
char *curl_easy_escape(CURL *c,const char *s,int n){size_t len=strlen(s);char *b=malloc(len*3+1),*p=b;for(size_t i=0;i<len;i++){unsigned char ch=s[i];if((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||(ch>='0'&&ch<='9'))*p++=ch;else{sprintf(p,"%%%02X",ch);p+=3;}}*p=0;return b;}
void curl_free(void *p){free(p);}
const char *curl_easy_strerror(CURLcode c){return "mock network error";}
CURLcode talk_tls_configure(CURL *c){return CURLE_OK;}
void talk_tls_diagnostic(char *p,size_t n){snprintf(p,n,"mock TLS");}
static TalkClient t;
int main(void){
 strcpy(t.server,"https://test.example");AlertRoom rooms[96];int count=0;
 assert(alert_fetch(&t,rooms,&count));assert(count==1 && rooms[0].id==52 && rooms[0].unread==4 && rooms[0].level==1 && !rooms[0].system && !rooms[0].silent && !strcmp(rooms[0].actor,"other") && !strcmp(t.account_id,"my-id"));assert(!strcmp(rooms[0].sender,"Alice") && !strcmp(rooms[0].body,"Hello Zayd, ready to play?"));
 step=1;assert(!alert_fetch(&t,rooms,&count) && strstr(t.error,"JSON"));
 step=2;assert(!alert_fetch(&t,rooms,&count) && strstr(t.error,"401"));
 step=3;assert(!alert_fetch(&t,rooms,&count) && strstr(t.error,"HTTPS"));
 step=4;assert(alert_fetch(&t,rooms,&count) && !rooms[0].system && !rooms[0].silent);
 step=5;assert(alert_fetch(&t,rooms,&count) && !rooms[0].system);
 step=6;assert(alert_fetch(&t,rooms,&count) && rooms[0].system);
 step=7;assert(alert_fetch(&t,rooms,&count) && rooms[0].silent && !rooms[0].system);
 QRMessage messages[QR_MESSAGES];step=8;assert(qr_http_history(&t,"newer",messages,&count));assert(count==2 && messages[0].id==1 && messages[1].id==2 && !strcmp(messages[1].body,"Hello Alice"));
 step=9;assert(qr_http_history(&t,"newer",messages,&count) && count==0);
 step=10;assert(!qr_http_history(&t,"newer",messages,&count) && strstr(t.error,"403"));
 assert(!qr_http_history(&t,"../bad",messages,&count));
 QRMessage sent;step=11;assert(qr_http_send(&t,"newer","Hello & +%",&sent) && sent.id==55 && !strcmp(sent.body,"Hello & +%"));
 step=12;assert(qr_http_send(&t,"newer","Hello & +%",&sent) && !strcmp(sent.body,"Hello & +%"));
 step=13;assert(!qr_http_send(&t,"newer","Hello & +%",&sent));
 step=14;assert(!qr_http_send(&t,"newer","Hello & +%",&sent));
 step=15;assert(!qr_http_send(&t,"newer","Hello & +%",&sent));
 assert(!qr_http_send(&t,"../bad","hi",&sent));assert(!qr_http_send(&t,"newer","",&sent));return 0;
}
