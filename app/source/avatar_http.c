#include "avatars.h"
#include "tls.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
typedef struct {unsigned char *data;size_t used;AvatarCancel cancel;void *context;} Download;
static size_t receive(char *p,size_t size,size_t count,void *context){
 Download *d=context;if(size && count>SIZE_MAX/size)return 0;size_t n=size*count;
 if(n>AVATAR_DOWNLOAD_LIMIT-d->used)return 0;memcpy(d->data+d->used,p,n);d->used+=n;return n;
}
static int progress(void *context,curl_off_t a,curl_off_t b,curl_off_t c,curl_off_t d){
 (void)a;(void)b;(void)c;(void)d;Download *download=context;return download->cancel && download->cancel(download->context);
}
bool avatar_download(const TalkClient *account,const AvatarKey *key,unsigned char **data,size_t *size,AvatarCancel cancel,void *context){
 *data=NULL;*size=0;if(strncmp(account->server,"https://",8) || (key->kind!=AVATAR_USER && key->kind!=AVATAR_ROOM) || !key->id[0] || !memchr(key->id,0,sizeof(key->id)) || !memchr(key->version,0,sizeof(key->version)))return false;
 CURL *curl=curl_easy_init();if(!curl)return false;
 char *id=curl_easy_escape(curl,key->id,0),*version=curl_easy_escape(curl,key->version,0);bool ok=false;
 Download download={.cancel=cancel,.context=context};download.data=malloc(AVATAR_DOWNLOAD_LIMIT);
 struct curl_slist *headers=NULL;
 if(!id || !version || !download.data)goto done;
 char url[2048];int n;
 if(key->kind==AVATAR_USER)n=snprintf(url,sizeof(url),"%s/index.php/avatar/%s/64?v=%s",account->server,id,version);
 else n=snprintf(url,sizeof(url),"%s/ocs/v2.php/apps/spreed/api/v1/room/%s/avatar?v=%s",account->server,id,version);
 if(n<0 || (size_t)n>=sizeof(url))goto done;
 headers=curl_slist_append(headers,"OCS-APIRequest: true");headers=curl_slist_append(headers,"Accept: image/png, image/jpeg");
 if(talk_tls_configure(curl)!=CURLE_OK)goto done;
 curl_easy_setopt(curl,CURLOPT_URL,url);curl_easy_setopt(curl,CURLOPT_HTTPHEADER,headers);
 curl_easy_setopt(curl,CURLOPT_USERNAME,account->username);curl_easy_setopt(curl,CURLOPT_PASSWORD,account->app_password);curl_easy_setopt(curl,CURLOPT_HTTPAUTH,CURLAUTH_BASIC);
 curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2L);curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,0L);
 curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT,2L);curl_easy_setopt(curl,CURLOPT_TIMEOUT,4L);curl_easy_setopt(curl,CURLOPT_NOSIGNAL,1L);
 curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,receive);curl_easy_setopt(curl,CURLOPT_WRITEDATA,&download);
 curl_easy_setopt(curl,CURLOPT_NOPROGRESS,0L);curl_easy_setopt(curl,CURLOPT_XFERINFOFUNCTION,progress);curl_easy_setopt(curl,CURLOPT_XFERINFODATA,&download);
 curl_easy_setopt(curl,CURLOPT_USERAGENT,"SwitchTalk/1.0.1 (libnx)");
 CURLcode result=curl_easy_perform(curl);long code=0;curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&code);
 ok=result==CURLE_OK && code>=200 && code<300 && download.used;
 if(ok){*data=download.data;*size=download.used;download.data=NULL;}
 done:free(download.data);if(id)curl_free(id);if(version)curl_free(version);curl_slist_free_all(headers);curl_easy_cleanup(curl);return ok;
}
