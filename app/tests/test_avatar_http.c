#include "avatars.h"
#include "tls.h"
#include <curl/curl.h>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
static char url[2048];static long follow,peer,host;static int scenario;static void *bodyctx,*progressctx;
static size_t (*bodyfn)(char*,size_t,size_t,void*);static int (*progressfn)(void*,curl_off_t,curl_off_t,curl_off_t,curl_off_t);
CURL *curl_easy_init(void){follow=peer=host=-1;return (void*)1;}void curl_easy_cleanup(CURL *p){(void)p;}
CURLcode curl_easy_setopt(CURL *c,CURLoption o,...){
 (void)c;va_list a;va_start(a,o);switch(o){
 case CURLOPT_URL:snprintf(url,sizeof(url),"%s",va_arg(a,char*));break;
 case CURLOPT_USERNAME:assert(!strcmp(va_arg(a,char*),"user"));break;
 case CURLOPT_PASSWORD:assert(!strcmp(va_arg(a,char*),"secret"));break;
 case CURLOPT_FOLLOWLOCATION:follow=va_arg(a,long);break;
 case CURLOPT_SSL_VERIFYPEER:peer=va_arg(a,long);break;
 case CURLOPT_SSL_VERIFYHOST:host=va_arg(a,long);break;
 case CURLOPT_WRITEFUNCTION:bodyfn=va_arg(a,void*);break;
 case CURLOPT_WRITEDATA:bodyctx=va_arg(a,void*);break;
 case CURLOPT_XFERINFOFUNCTION:progressfn=va_arg(a,void*);break;
 case CURLOPT_XFERINFODATA:progressctx=va_arg(a,void*);break;
 default:break;}va_end(a);return CURLE_OK;
}
CURLcode curl_easy_getinfo(CURL *c,CURLINFO i,...){(void)c;(void)i;va_list a;va_start(a,i);*va_arg(a,long*)=scenario==1?404:(scenario==2?302:200);va_end(a);return CURLE_OK;}
CURLcode curl_easy_perform(CURL *c){
 (void)c;assert(peer==1 && host==2 && follow==0);
 if(scenario==3)return CURLE_PEER_FAILED_VERIFICATION;
 if(scenario==4){assert(!bodyfn("x",1,AVATAR_DOWNLOAD_LIMIT+1,bodyctx));return CURLE_WRITE_ERROR;}
 if(progressfn(progressctx,0,0,0,0))return CURLE_ABORTED_BY_CALLBACK;
 assert(bodyfn("PNG",1,3,bodyctx)==3);return CURLE_OK;
}
char *curl_easy_escape(CURL *c,const char *s,int n){(void)c;(void)n;char *out=malloc(strlen(s)*3+1),*p=out;for(;*s;s++){if(isalnum((unsigned char)*s)||*s=='-')*p++=*s;else{sprintf(p,"%%%02X",(unsigned char)*s);p+=3;}}*p=0;return out;}
void curl_free(void *p){free(p);}struct curl_slist *curl_slist_append(struct curl_slist *p,const char *s){(void)p;(void)s;return (void*)1;}void curl_slist_free_all(struct curl_slist *p){(void)p;}
CURLcode talk_tls_configure(CURL *c){(void)c;return CURLE_OK;}
static bool cancel(void *p){return *(bool*)p;}
int main(void){
 static TalkClient t;strcpy(t.server,"https://cloud.example/base");strcpy(t.username,"user");strcpy(t.app_password,"secret");
 AvatarKey key={.kind=AVATAR_USER};strcpy(key.id,"id/@name");strcpy(key.version,"1&2");unsigned char *data;size_t n;bool stop=false;
 assert(avatar_download(&t,&key,&data,&n,cancel,&stop));assert(n==3 && !memcmp(data,"PNG",3));free(data);assert(strstr(url,"/base/index.php/avatar/id%2F%40name/64?v=1%262"));
 key.kind=AVATAR_ROOM;assert(avatar_download(&t,&key,&data,&n,cancel,&stop));free(data);assert(strstr(url,"/api/v1/room/id%2F%40name/avatar"));
 for(scenario=1;scenario<=4;scenario++){assert(!avatar_download(&t,&key,&data,&n,cancel,&stop));assert(!data && !n);}
 scenario=0;stop=true;assert(!avatar_download(&t,&key,&data,&n,cancel,&stop));
 strcpy(t.server,"http://insecure");assert(!avatar_download(&t,&key,&data,&n,cancel,&stop));
 puts("Avatar HTTP: Nextcloud subpath, encoded IDs/versions, basic auth, strict HTTPS, no redirects, bounded body, cancellation and nonfatal errors: PASS");return 0;
}
