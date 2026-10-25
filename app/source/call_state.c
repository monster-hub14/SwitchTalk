#include "call_state.h"
#include <string.h>
static bool text(const char *p,size_t n){return memchr(p,0,n)!=NULL;}
bool call_validate_request(const CallRequest *s){
 if(s->version!=CALL_VERSION || !text(s->server,sizeof(s->server)) || !text(s->username,sizeof(s->username)) || !text(s->password,sizeof(s->password)) || !text(s->room,sizeof(s->room)) || !text(s->title,sizeof(s->title)))return false;
 if(strncmp(s->server,"https://",8) || !s->server[8] || !s->username[0] || !s->password[0])return false;
 size_t n=strlen(s->room);if(n<4 || n>30)return false;
 for(size_t i=0;i<n;i++)if(!((s->room[i]>='a' && s->room[i]<='z') || (s->room[i]>='0' && s->room[i]<='9')))return false;
 return true;
}
void call_erase(void *memory,size_t size){volatile unsigned char *p=memory;while(size--)*p++=0;}
