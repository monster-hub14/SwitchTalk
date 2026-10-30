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

#define RESPONSE_LIMIT (512*1024)

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
static long response_cursor,last_http_status;
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
    response_cursor=0;last_http_status=0;
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
    curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&status);last_http_status=status;
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
        set_error(t,doc.used>=JSON_MAX_TOKENS?"Conversation JSON exceeds test token limit":"Invalid API JSON response");
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
#include "alert_policy.h"
bool alert_fetch(TalkClient *t,AlertRoom *rooms,int *count){
 int data=request(t,"/ocs/v2.php/apps/spreed/api/v4/room?format=json&noStatusUpdate=1",NULL,false);
 if(data<0)return false;
 if(doc.tokens[data].type!=JSON_ARRAY){set_error(t,"Unexpected conversation response");return false;}
 int n=json_count(&doc,data);if(n>ALERT_ROOMS){set_error(t,"Test supports up to 96 conversations");return false;}
 *count=0;
 for(int i=0;i<n;i++){
  int row=json_item(&doc,data,i);AlertRoom *r=&rooms[(*count)++];memset(r,0,sizeof(*r));
  extract_field(&doc,row,"token",r->token,sizeof(r->token));extract_field(&doc,row,"displayName",r->title,sizeof(r->title));
  r->unread=json_long(&doc,json_get(&doc,row,"unreadMessages"));r->level=json_long(&doc,json_get(&doc,row,"notificationLevel"));r->type=json_long(&doc,json_get(&doc,row,"type"));
  int last=json_get(&doc,row,"lastMessage");r->id=json_long(&doc,json_get(&doc,last,"id"));
  extract_field(&doc,last,"actorDisplayName",r->sender,sizeof(r->sender));talk_format_message(&doc,last,r->body,sizeof(r->body));
  char type[32],event[128];extract_field(&doc,last,"actorType",type,sizeof(type));
  if(!strcmp(type,"users"))extract_field(&doc,last,"actorId",r->actor,sizeof(r->actor));
  extract_field(&doc,last,"systemMessage",event,sizeof(event));int system=json_get(&doc,last,"systemMessage");r->system=system>=0 && doc.tokens[system].type==JSON_STRING && event[0]!=0;
  extract_field(&doc,last,"messageType",type,sizeof(type));r->system=r->system || !strcmp(type,"system");
  int silent=json_get(&doc,last,"silent");r->silent=!strcmp(type,"silent") || (silent>=0 && doc.tokens[silent].type==JSON_PRIMITIVE && doc.tokens[silent].end-doc.tokens[silent].start==4 && !memcmp(doc.text+doc.tokens[silent].start,"true",4));
  extract_field(&doc,row,"actorType",type,sizeof(type));if(!strcmp(type,"users"))extract_field(&doc,row,"actorId",t->account_id,sizeof(t->account_id));
 }
 return true;
}

#include "qr_http.h"
static bool valid_room_token(const char *s){size_t n=strlen(s);if(!n || n>=128)return false;for(size_t i=0;i<n;i++)if(!((s[i]>='a'&&s[i]<='z')||(s[i]>='A'&&s[i]<='Z')||(s[i]>='0'&&s[i]<='9')||s[i]=='-'||s[i]=='_'))return false;return true;}
static void qr_parse_message(int row,QRMessage *m){memset(m,0,sizeof(*m));m->id=json_long(&doc,json_get(&doc,row,"id"));extract_field(&doc,row,"actorDisplayName",m->sender,sizeof(m->sender));size_t n=strlen(m->sender);if(n){size_t start=n-1;while(start && ((unsigned char)m->sender[start]&0xc0)==0x80)--start;unsigned char c=m->sender[start];size_t need=c<128?1:c>=0xc2&&c<=0xdf?2:c>=0xe0&&c<=0xef?3:c>=0xf0&&c<=0xf4?4:1;if(n-start<need)m->sender[start]=0;}if(!m->sender[0])snprintf(m->sender,sizeof(m->sender),"Someone");talk_format_message(&doc,row,m->body,sizeof(m->body));}
bool qr_http_history(TalkClient *t,const char *token,QRMessage *out,int *count){
 *count=0;if(!valid_room_token(token)){set_error(t,"Invalid conversation token");return false;}
 char path[400];snprintf(path,sizeof(path),"/ocs/v2.php/apps/spreed/api/v1/chat/%s?lookIntoFuture=0&limit=10&setReadMarker=0&markNotificationsAsRead=0&noStatusUpdate=1&format=json",token);
 int data=request(t,path,NULL,true);if(data==-2)return true;if(data<0)return false;
 if(doc.tokens[data].type!=JSON_ARRAY){set_error(t,"Unexpected message response");return false;}
 int n=json_count(&doc,data);if(n>QR_MESSAGES)n=QR_MESSAGES;
 for(int i=0;i<n;i++){qr_parse_message(json_item(&doc,data,i),&out[i]);}
 for(int i=1;i<n;i++){QRMessage m=out[i];int j=i;while(j>0 && out[j-1].id>m.id){out[j]=out[j-1];--j;}out[j]=m;}
 *count=n;return true;
}
bool qr_http_send(TalkClient *t,const char *token,const char *text,QRMessage *out){
 if(!valid_room_token(token) || !text[0] || strlen(text)>QR_TEXT){set_error(t,"Reply must contain 1-512 bytes");return false;}
 CURL *curl=curl_easy_init();if(!curl){set_error(t,"HTTP allocation failed");return false;}
 char *encoded=curl_easy_escape(curl,text,0);curl_easy_cleanup(curl);if(!encoded){set_error(t,"Reply encoding failed");return false;}
 size_t size=strlen(encoded)+9;char *form=malloc(size);if(!form){curl_free(encoded);set_error(t,"Reply allocation failed");return false;}
 snprintf(form,size,"message=%s",encoded);curl_free(encoded);char path[240];snprintf(path,sizeof(path),"/ocs/v2.php/apps/spreed/api/v1/chat/%s?format=json",token);
 int data=request(t,path,form,false);free(form);
 if(last_http_status!=201){if(!t->error[0])set_error(t,"Send not confirmed by server");return false;}
 memset(out,0,sizeof(*out));if(data>=0 && doc.tokens[data].type==JSON_OBJECT)qr_parse_message(data,out);
 if(!out->body[0])snprintf(out->body,sizeof(out->body),"%s",text);snprintf(out->sender,sizeof(out->sender),"You");t->error[0]=0;return true;
}
