#include "alert_preview.h"
#include "notify_file.h"
#include <assert.h>
#include <string.h>
int main(void){
 AlertRoom r={.token="preview",.title="Friends",.sender="Alice",.body="Hey, ready to play?",.type=1};char text[512];
 alert_preview(&r,text,sizeof(text));assert(!strcmp(text,"Alice\nHey, ready to play?"));
 assert(notify_write("preview",1,text));
 r.type=2;alert_preview(&r,text,sizeof(text));assert(!strcmp(text,"Friends / Alice\nHey, ready to play?"));
 r.sender[0]=0;strcpy(r.actor,"user-id");r.body[0]=0;alert_preview(&r,text,sizeof(text));assert(strstr(text,"user-id") && strstr(text,"Shared a message or attachment"));
 r.type=1;strcpy(r.sender,"Alice \" \\ ");strcpy(r.body,"Bonjour, \xe4\xbd\xa0\xe5\xa5\xbd \xf0\x9f\x98\x80");alert_preview(&r,text,sizeof(text));assert(notify_write("preview",2,text));
 memset(r.body,'x',239);r.body[239]=(char)0xe2;r.body[240]=(char)0x82;r.body[241]=(char)0xac;r.body[242]=0;alert_preview(&r,text,sizeof(text));assert(strstr(text,"...") && !strchr(text,(char)0xe2));
 char guarded[8];memset(guarded,'Z',sizeof(guarded));alert_preview(&r,guarded+1,4);assert(guarded[0]=='Z' && guarded[5]=='Z' && guarded[6]=='Z' && guarded[4]==0);
 return 0;
}
