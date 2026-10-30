#include "talk.h"
#include "json.h"
#include "tls.h"
#include "server_url.h"
#include "rich_text.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define RESPONSE_LIMIT (1024*1024)

typedef struct {char *buf;size_t len;size_t max;} HttpBody;
static size_t receive_body(char *p,size_t size,size_t n,void *ctx) {
    HttpBody *b=(HttpBody*)ctx;
    if(size && n>SIZE_MAX/size)return 0;
    size_t len=size*n;
    if(len>b->max-b->len-1) return 0;
    memcpy(b->buf+b->len,p,len);b->len+=len;b->buf[b->len]=0;
    return len;
}
static void set_error(TalkClient *t,const char *s){snprintf(t->error,sizeof(t->error),"%s",s);}
static void extract_field(const JsonDoc *d,int obj,const char *key,char *out,size_t cap){json_string(d,json_get(d,obj,key),out,cap);}
static JsonDoc doc;
static char s_http_response[RESPONSE_LIMIT];
static bool (*cancel_request)(void*);
static void *cancel_ctx;
void talk_set_cancel(bool (*cancel)(void*),void *ctx){cancel_request=cancel;cancel_ctx=ctx;}
static int progress(void *ctx,curl_off_t a,curl_off_t b,curl_off_t c,curl_off_t d){
    (void)ctx;(void)a;(void)b;(void)c;(void)d;
    return cancel_request && cancel_request(cancel_ctx);
}
static long response_cursor;
static unsigned poll_wait=8;
void talk_set_poll_wait(unsigned seconds){poll_wait=seconds>=1 && seconds<=8?seconds:8;}
static size_t receive_header(char *p,size_t size,size_t n,void *ctx){
    (void)ctx;
    size_t len=size*n;
    const char key[]="X-Chat-Last-Given:";
    if(len>=sizeof(key)-1 && !strncasecmp(p,key,sizeof(key)-1)){
        char val[40];size_t count=len-(sizeof(key)-1);
        if(count>=sizeof(val))count=sizeof(val)-1;
        memcpy(val,p+sizeof(key)-1,count);val[count]=0;
        response_cursor=strtol(val,NULL,10);
    }
    return len;
}
static int request(TalkClient *t,const char *path,const char *post,bool idle_ok) {
    response_cursor=0;
    t->error[0]=0;
    size_t base=strlen(t->server),plen=strlen(path);
    if(base+plen+2>2048){set_error(t,"Server URL is too long");return -1;}
    char url[2048];
    memcpy(url,t->server,base);
    memcpy(url+base,path,plen+1);
    char detail[CURL_ERROR_SIZE]={0};
    CURL *curl=curl_easy_init();
    if(!curl){set_error(t,"Cannot initialize HTTP client");return -1;}
    HttpBody b={0};b.max=RESPONSE_LIMIT;b.buf=s_http_response;
    b.buf[0]=0;
    struct curl_slist *headers=NULL;
    headers=curl_slist_append(headers,"OCS-APIRequest: true");
    headers=curl_slist_append(headers,"Accept: application/json");
    if(post)headers=curl_slist_append(headers,"Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(curl,CURLOPT_URL,url);
    curl_easy_setopt(curl,CURLOPT_ERRORBUFFER,detail);
    CURLcode tls_setup=talk_tls_configure(curl);
    if(tls_setup!=CURLE_OK){
        set_error(t,"TLS trust configuration failed");
        curl_slist_free_all(headers);curl_easy_cleanup(curl);return -1;
    }
    curl_easy_setopt(curl,CURLOPT_HTTPHEADER,headers);
    curl_easy_setopt(curl,CURLOPT_USERNAME,t->username);
    curl_easy_setopt(curl,CURLOPT_PASSWORD,t->app_password);
    curl_easy_setopt(curl,CURLOPT_HTTPAUTH,CURLAUTH_BASIC);
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L);
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2L);
    curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,0L); /* Do not forward credentials to redirects */
    curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT,10L);
    curl_easy_setopt(curl,CURLOPT_TIMEOUT,25L);
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,receive_body);
    curl_easy_setopt(curl,CURLOPT_WRITEDATA,&b);
    curl_easy_setopt(curl,CURLOPT_HEADERFUNCTION,receive_header);
    curl_easy_setopt(curl,CURLOPT_NOPROGRESS,0L);
    curl_easy_setopt(curl,CURLOPT_XFERINFOFUNCTION,progress);
    curl_easy_setopt(curl,CURLOPT_NOSIGNAL,1L);
    curl_easy_setopt(curl,CURLOPT_USERAGENT,"SwitchTalk/1.1.0 (libnx)");
    if(post){curl_easy_setopt(curl,CURLOPT_POSTFIELDS,post);curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE,(long)strlen(post));}
    CURLcode result=curl_easy_perform(curl);long status=0;
    curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&status);
    if(result==CURLE_OK && status==304 && idle_ok){
        curl_slist_free_all(headers);curl_easy_cleanup(curl);return -2;
    }
    if(result!=CURLE_OK) {
        if(result==CURLE_PEER_FAILED_VERIFICATION || result==CURLE_SSL_CONNECT_ERROR || result==CURLE_SSL_CACERT_BADFILE) {
            char tls_detail[160];talk_tls_diagnostic(tls_detail,sizeof(tls_detail));
            snprintf(t->error,sizeof(t->error),"HTTPS (%d): %s",(int)result,tls_detail);
        } else {
            snprintf(t->error,sizeof(t->error),"Network (%d): %.210s",
                     (int)result,detail[0]?detail:curl_easy_strerror(result));
        }
    } else if(status<200 || status>=300) {
        snprintf(t->error,sizeof(t->error),"Server returned HTTP %ld (check URL/login)",status);
    } else if(json_parse(&doc,b.buf)!=0) {
        set_error(t,"Invalid API JSON response (check Talk installed)");
    } else {
        int ocs=json_get(&doc,0,"ocs");
        int meta=json_get(&doc,ocs,"meta");
        long code=json_long(&doc,json_get(&doc,meta,"statuscode"));
        if(ocs<0 || (code!=100 && (code<200 || code>=300))) {
            char msg[130];extract_field(&doc,meta,"message",msg,sizeof(msg));
            snprintf(t->error,sizeof(t->error),"Talk API: %s (%ld)",msg[0]?msg:"request failed",code);
        }
    }
    curl_slist_free_all(headers);curl_easy_cleanup(curl);
    if(t->error[0])return -1;
    int ocs=json_get(&doc,0,"ocs");
    int data=json_get(&doc,ocs,"data");
    if(data<0)set_error(t,"Talk API response is missing data");
    return data;
}
void talk_init(TalkClient *t){memset(t,0,sizeof(*t));}
void talk_clear_password(TalkClient *t){volatile char *p=(volatile char*)t->app_password;for(size_t i=0;i<sizeof(t->app_password);i++)p[i]=0;}
bool talk_login(TalkClient *t) {
    if(!server_normalize(t->server,sizeof(t->server))) {set_error(t,"Use an HTTPS URL (trusted certificate required)");return false;}
    size_t n=strlen(t->server);while(n>8&&t->server[n-1]=='/')t->server[--n]=0;
    if(!t->username[0]||!t->app_password[0]){set_error(t,"Enter username and password");return false;}
    return talk_rooms(t);
}
bool talk_rooms(TalkClient *t) {
    int data=request(t,"/ocs/v2.php/apps/spreed/api/v4/room?format=json",NULL,false);
    if(data<0)return false;
    if(doc.tokens[data].type!=JSON_ARRAY){set_error(t,"Unexpected conversations response");return false;}
    t->room_count=0;
    int n=json_count(&doc,data);
    for(int i=0;i<n && t->room_count<TALK_ROOMS_MAX;i++) {
        int row=json_item(&doc,data,i);
        TalkRoom *room=&t->rooms[t->room_count];memset(room,0,sizeof(*room));
        extract_field(&doc,row,"token",room->token,sizeof(room->token));
        extract_field(&doc,row,"displayName",room->title,sizeof(room->title));
        if(!room->title[0])extract_field(&doc,row,"name",room->title,sizeof(room->title));
        char kind[32],actor[128];extract_field(&doc,row,"actorType",kind,sizeof(kind));extract_field(&doc,row,"actorId",actor,sizeof(actor));
        if(!strcmp(kind,"users") && actor[0])snprintf(t->account_id,sizeof(t->account_id),"%s",actor);
        int type=(int)json_long(&doc,json_get(&doc,row,"type"));
        if(type==1)extract_field(&doc,row,"name",room->avatar_user,sizeof(room->avatar_user));
        else if(type==6)snprintf(room->avatar_user,sizeof(room->avatar_user),"%s",t->account_id[0]?t->account_id:t->username);
        extract_field(&doc,row,"avatarVersion",room->avatar_version,sizeof(room->avatar_version));
        int custom=json_get(&doc,row,"isCustomAvatar");
        room->custom_avatar=custom>=0 && doc.tokens[custom].type==JSON_PRIMITIVE && doc.tokens[custom].end-doc.tokens[custom].start==4 && !memcmp(doc.text+doc.tokens[custom].start,"true",4);
        room->unread=(int)json_long(&doc,json_get(&doc,row,"unreadMessages"));
        if(!room->title[0])snprintf(room->title,sizeof(room->title),"Conversation %d",i+1);
        if(room->token[0])t->room_count++;
    }
    return true;
}
static void parse_message(int row,TalkMessage *m){
    memset(m,0,sizeof(*m));
    m->id=json_long(&doc,json_get(&doc,row,"id"));
    extract_field(&doc,row,"actorDisplayName",m->sender,sizeof(m->sender));
    char actor_type[32];extract_field(&doc,row,"actorType",actor_type,sizeof(actor_type));
    if(!strcmp(actor_type,"users"))extract_field(&doc,row,"actorId",m->sender_id,sizeof(m->sender_id));
    talk_format_message(&doc,row,m->body,sizeof(m->body));
    if(!m->sender[0])snprintf(m->sender,sizeof(m->sender),"System");
}
static bool merge_response(TalkClient *t,int data,bool history){
    if(doc.tokens[data].type!=JSON_ARRAY){set_error(t,"Unexpected messages response");return false;}
    if(history){t->message_count=0;t->poll_cursor=0;}
    int n=json_count(&doc,data);
    for(int i=0;i<n;i++){
        TalkMessage m;parse_message(json_item(&doc,data,i),&m);
        talk_merge_message(t,&m);
        if(m.id>t->poll_cursor)t->poll_cursor=m.id;
    }
    /* History's header points backward; forward polling's header points forward. */
    if(!history && response_cursor>t->poll_cursor)t->poll_cursor=response_cursor;
    return true;
}
bool talk_messages(TalkClient *t,const char *token) {
    char path[360];snprintf(path,sizeof(path),"/ocs/v2.php/apps/spreed/api/v1/chat/%s?lookIntoFuture=0&limit=60&format=json",token);
    int data=request(t,path,NULL,true);
    if(data==-2){t->message_count=0;t->poll_cursor=0;return true;}
    return data>=0 && merge_response(t,data,true);
}
bool talk_poll(TalkClient *t,const char *token){
    char path[440];
    snprintf(path,sizeof(path),"/ocs/v2.php/apps/spreed/api/v1/chat/%s?lookIntoFuture=1&lastKnownMessageId=%ld&includeLastKnown=0&limit=100&timeout=%u&format=json",token,t->poll_cursor,poll_wait);
    int data=request(t,path,NULL,true);
    return data==-2 || (data>=0 && merge_response(t,data,false));
}
bool talk_send(TalkClient *t,const char *token,const char *message) {
    if(!message[0]){set_error(t,"Empty message");return false;}
    CURL *c=curl_easy_init();if(!c){set_error(t,"Cannot encode message");return false;}
    char *encoded=curl_easy_escape(c,message,0);
    if(!encoded){curl_easy_cleanup(c);set_error(t,"Cannot encode message");return false;}
    char *form=(char*)malloc(strlen(encoded)+9);
    if(!form){curl_free(encoded);curl_easy_cleanup(c);set_error(t,"Out of memory");return false;}
    sprintf(form,"message=%s",encoded);
    curl_free(encoded);curl_easy_cleanup(c);
    char path[320];snprintf(path,sizeof(path),"/ocs/v2.php/apps/spreed/api/v1/chat/%s?format=json",token);
    int status=request(t,path,form,false);
    free(form);
    if(status<0)return false;
    /* The POST is confirmed. Do not turn a later refresh failure into a failed send. */
    if(doc.tokens[status].type==JSON_OBJECT){
        TalkMessage m;parse_message(status,&m);talk_merge_message(t,&m);
    }
    return true;
}
