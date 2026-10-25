#include "server_url.h"
#include <string.h>
#include <ctype.h>
#include <strings.h>
bool server_normalize(char *server,size_t capacity){
 if(!server || !capacity || !memchr(server,0,capacity))return false;
 char clean[512];size_t n=strlen(server);const char *start=server;
 while(n && isspace((unsigned char)*start)){++start;--n;}
 while(n && isspace((unsigned char)start[n-1]))--n;
 if(!n || n>=sizeof(clean))return false;
 memcpy(clean,start,n);clean[n]=0;
 const char *host=clean;
 if(!strncasecmp(host,"https://",8))host+=8;
 else if(strstr(host,"://"))return false;
 const char *end=strchr(host,'/');size_t hostlen=end?(size_t)(end-host):strlen(host);
 if(!hostlen || host[0]=='.' || host[0]==':' || host[hostlen-1]==':')return false;
 for(const char *p=host;*p;p++)if(isspace((unsigned char)*p) || *p=='@' || *p=='?' || *p=='#' || *p=='\\' || (unsigned char)*p<32)return false;
 if(strstr(host,"://"))return false;
 size_t len=strlen(host);while(len && host[len-1]=='/')--len;
 if(len+9>capacity)return false;
 memcpy(server,"https://",8);memcpy(server+8,host,len);server[8+len]=0;return true;
}
bool server_has_host(const char *server){char copy[512];size_t n=strlen(server);if(n>=sizeof(copy))return false;memcpy(copy,server,n+1);return server_normalize(copy,sizeof(copy));}
