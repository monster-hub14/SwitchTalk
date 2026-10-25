#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "call_http.h"
#include "tls.h"
static CURL *http;
static const TalkClient *account;
static char response[1024*1024];static size_t response_len;
static bool cleanup_http,session_auth;
void call_http_cleanup_mode(void){cleanup_http=true;}
const char *call_json_string(const cJSON *o,const char *key){cJSON *v=cJSON_GetObjectItemCaseSensitive(o,key);return cJSON_IsString(v)?v->valuestring:"";}
static int cancel(void *ctx,curl_off_t a,curl_off_t b,curl_off_t c,curl_off_t d){(void)ctx;(void)a;(void)b;(void)c;(void)d;return call_cancelled();}
static size_t body(char *p,size_t size,size_t count,void *ctx){
    (void)ctx;if(size && count>SIZE_MAX/size)return 0;size_t n=size*count;
    if(n>=sizeof(response)-response_len)return 0;
    memcpy(response+response_len,p,n);response_len+=n;response[response_len]=0;return n;
}
void call_http_init(const TalkClient *client){
    account=client;cleanup_http=session_auth=false;http=curl_easy_init();
    if(http){
        curl_easy_setopt(http,CURLOPT_COOKIEFILE,"");
        /* Nextcloud probes this marker before creating a browser session token.
         * The cookie jar stays in RAM and belongs only to this Nextcloud handle. */
        curl_easy_setopt(http,CURLOPT_COOKIE,"cookie_test=test");
    }
}
void call_http_use_session(void){session_auth=true;}

void call_http_close(void){if(http)curl_easy_cleanup(http);http=NULL;account=NULL;memset(response,0,sizeof(response));}
cJSON *call_http(const char *method,const char *path,const char *form,char *error,size_t cap){
    if(!http){snprintf(error,cap,"Cannot initialize call HTTP");return NULL;}
    char url[2048],detail[CURL_ERROR_SIZE]={0};
    if(strlen(account->server)+strlen(path)>=sizeof(url)){snprintf(error,cap,"Call URL too long");return NULL;}
    strcpy(url,account->server);strcat(url,path);response_len=0;response[0]=0;
    /* Retain the authenticated cookie session throughout room/call requests.
     * Clear each request body explicitly so GET/DELETE cannot reuse POST data. */
    curl_easy_setopt(http,CURLOPT_POSTFIELDS,NULL);
    curl_easy_setopt(http,CURLOPT_HTTPGET,1L);
    struct curl_slist *headers=NULL;
    headers=curl_slist_append(headers,"OCS-APIRequest: true");headers=curl_slist_append(headers,"Accept: application/json");headers=curl_slist_append(headers,"Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(http,CURLOPT_URL,url);
    curl_easy_setopt(http,CURLOPT_HTTPHEADER,headers);curl_easy_setopt(http,CURLOPT_USERNAME,session_auth?NULL:account->username);curl_easy_setopt(http,CURLOPT_PASSWORD,session_auth?NULL:account->app_password);curl_easy_setopt(http,CURLOPT_HTTPAUTH,session_auth?CURLAUTH_NONE:CURLAUTH_BASIC);
    curl_easy_setopt(http,CURLOPT_ERRORBUFFER,detail);curl_easy_setopt(http,CURLOPT_CONNECTTIMEOUT,cleanup_http?3L:10L);curl_easy_setopt(http,CURLOPT_TIMEOUT,cleanup_http?4L:20L);
    curl_easy_setopt(http,CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(http,CURLOPT_SSL_VERIFYHOST,2L);
    curl_easy_setopt(http,CURLOPT_WRITEFUNCTION,body);curl_easy_setopt(http,CURLOPT_WRITEDATA,NULL);
    curl_easy_setopt(http,CURLOPT_NOPROGRESS,0L);curl_easy_setopt(http,CURLOPT_XFERINFOFUNCTION,cancel);curl_easy_setopt(http,CURLOPT_NOSIGNAL,1L);
    if(talk_tls_configure(http)!=CURLE_OK){curl_slist_free_all(headers);snprintf(error,cap,"Cannot configure call TLS trust");return NULL;}
    if(form)curl_easy_setopt(http,CURLOPT_POSTFIELDS,form);
    curl_easy_setopt(http,CURLOPT_CUSTOMREQUEST,method);
    CURLcode rc=curl_easy_perform(http);long status=0;curl_easy_getinfo(http,CURLINFO_RESPONSE_CODE,&status);curl_slist_free_all(headers);
    if(rc!=CURLE_OK){snprintf(error,cap,"Call HTTP: %.150s",detail[0]?detail:curl_easy_strerror(rc));return NULL;}
    if(status<200 || status>=300){
        cJSON *failure=cJSON_Parse(response),*ocs=cJSON_GetObjectItemCaseSensitive(failure,"ocs"),*meta=cJSON_GetObjectItemCaseSensitive(ocs,"meta");
        snprintf(error,cap,"Call %s HTTP %ld: %.70s at %.95s",method,status,call_json_string(meta,"message"),path);
        cJSON_Delete(failure);return NULL;
    }
    cJSON *root=cJSON_Parse(response),*ocs=cJSON_GetObjectItemCaseSensitive(root,"ocs"),*meta=cJSON_GetObjectItemCaseSensitive(ocs,"meta"),*code=cJSON_GetObjectItemCaseSensitive(meta,"statuscode");
    int value=cJSON_IsNumber(code)?code->valueint:0;
    if(value!=100 && (value<200 || value>=300)){
        snprintf(error,cap,"Call API %d: %.150s",value,call_json_string(meta,"message"));cJSON_Delete(root);return NULL;
    }
    cJSON *data=cJSON_DetachItemFromObjectCaseSensitive(ocs,"data");cJSON_Delete(root);
    if(!data)snprintf(error,cap,"Missing call API data");return data;
}
bool call_http_check_room_session(const char *token,const char *expected,char *error,size_t cap){
    char path[512];
    if(strlen(token)>128){snprintf(error,cap,"Room token too long");return false;}
    snprintf(path,sizeof(path),"/ocs/v2.php/apps/spreed/api/v4/room/%s?format=json",token);
    cJSON *room=call_http("GET",path,NULL,error,cap);if(!room)return false;
    bool matches=expected[0] && !strcmp(call_json_string(room,"sessionId"),expected);
    cJSON_Delete(room);
    if(!matches)snprintf(error,cap,"Nextcloud did not retain the joined room session. Check proxy session cookies.");
    return matches;
}
