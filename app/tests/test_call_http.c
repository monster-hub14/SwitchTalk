#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "call_http.h"
#include "tls.h"
static char url[2048],method[16],form[128],cookie[64];
static const char *username,*password;
static long auth,status;static int step,closes;
static size_t (*write_fn)(char*,size_t,size_t,void*);static void *write_ctx;
bool call_cancelled(void){return false;}
void call_set_status(const char *s){(void)s;}
CURL *curl_easy_init(void){return (CURL*)1;}
void curl_easy_cleanup(CURL *p){++closes;}
CURLcode curl_easy_setopt(CURL *p,CURLoption opt,...){
    va_list a;va_start(a,opt);
    switch(opt){
    case CURLOPT_URL:snprintf(url,sizeof(url),"%s",va_arg(a,char*));break;
    case CURLOPT_CUSTOMREQUEST:snprintf(method,sizeof(method),"%s",va_arg(a,char*));break;
    case CURLOPT_POSTFIELDS:{const char *s=va_arg(a,char*);snprintf(form,sizeof(form),"%s",s?s:"");break;}
    case CURLOPT_COOKIE:snprintf(cookie,sizeof(cookie),"%s",va_arg(a,char*));break;
    case CURLOPT_USERNAME:username=va_arg(a,char*);break;
    case CURLOPT_PASSWORD:password=va_arg(a,char*);break;
    case CURLOPT_HTTPAUTH:auth=va_arg(a,long);break;
    case CURLOPT_WRITEFUNCTION:write_fn=va_arg(a,void*);break;
    case CURLOPT_WRITEDATA:write_ctx=va_arg(a,void*);break;
    default:break;
    }
    va_end(a);return CURLE_OK;
}
/* A reset used to obscure persistent-session setup; this path must keep it. */
void curl_easy_reset(CURL *p){assert(0);}
CURLcode curl_easy_getinfo(CURL *p,CURLINFO info,...){va_list a;va_start(a,info);*va_arg(a,long*)=status;va_end(a);return CURLE_OK;}
struct curl_slist *curl_slist_append(struct curl_slist *p,const char *s){return (void*)1;}
void curl_slist_free_all(struct curl_slist *p){}
const char *curl_easy_strerror(CURLcode c){return "mock failure";}
CURLcode talk_tls_configure(CURL *p){return CURLE_OK;}
CURLcode curl_easy_perform(CURL *p){
    const char *data="null";status=200;
    assert(!strcmp(cookie,"cookie_test=test"));
    if(step==0 || step==9){assert(username && password && auth==CURLAUTH_BASIC);assert(!strcmp(method,"GET"));data="{}";}
    else {
        /* Model the server rejecting a new Basic-auth session after room join. */
        assert(!username && !password && auth==CURLAUTH_NONE);
        if(step==1){assert(!strcmp(method,"POST") && !strcmp(form,"force=false"));data="{\"sessionId\":\"room-session\"}";}
        if(step==2){assert(!strcmp(method,"GET") && !form[0]);data="{\"sessionId\":\"room-session\"}";}
        if(step==3){assert(!strcmp(method,"POST") && !strcmp(form,"flags=3") && strstr(url,"/v4/call/"));}
        if(step==4){assert(!strcmp(method,"PUT") && !strcmp(form,"flags=1"));}
        if(step==5){assert(!strcmp(method,"DELETE") && !form[0]);}
        if(step==6){status=404;const char *body="{\"ocs\":{\"meta\":{\"statuscode\":404,\"message\":\"Room session missing\"},\"data\":null}}";write_fn((char*)body,1,strlen(body),write_ctx);return CURLE_OK;}
        if(step==7){data="{\"sessionId\":\"different-session\"}";}
    }
    char body[1024];snprintf(body,sizeof(body),"{\"ocs\":{\"meta\":{\"statuscode\":200},\"data\":%s}}",data);write_fn(body,1,strlen(body),write_ctx);return CURLE_OK;
}
int main(void){
    TalkClient t;memset(&t,0,sizeof(t));strcpy(t.server,"https://example.test");strcpy(t.username,"user");strcpy(t.app_password,"password");char error[240]={0};cJSON *d;
    call_http_init(&t);step=0;d=call_http("GET","/settings",NULL,error,sizeof(error));assert(d);cJSON_Delete(d);
    call_http_use_session();step=1;d=call_http("POST","/room/active","force=false",error,sizeof(error));assert(d);cJSON_Delete(d);
    step=2;assert(call_http_check_room_session("token","room-session",error,sizeof(error)));
    step=3;d=call_http("POST","/ocs/v2.php/apps/spreed/api/v4/call/token","flags=3",error,sizeof(error));assert(cJSON_IsNull(d));cJSON_Delete(d);
    step=4;d=call_http("PUT","/call","flags=1",error,sizeof(error));assert(d);cJSON_Delete(d);
    step=5;d=call_http("DELETE","/call",NULL,error,sizeof(error));assert(d);cJSON_Delete(d);
    step=6;assert(!call_http("POST","/call","flags=3",error,sizeof(error)));assert(strstr(error,"POST HTTP 404") && strstr(error,"Room session missing"));
    step=7;assert(!call_http_check_room_session("token","room-session",error,sizeof(error)));assert(strstr(error,"did not retain"));
    call_http_close();assert(closes==1);call_http_init(&t);step=9;d=call_http("GET","/settings",NULL,error,sizeof(error));assert(d);cJSON_Delete(d);call_http_close();assert(closes==2);
    puts("Call HTTP: session bootstrap, cookie-only room/call, GET/body reset, mute/hangup, null success, 404 detail and session mismatch: PASS");return 0;
}
