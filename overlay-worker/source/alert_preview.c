/* Plain UTF-8 preview; no message body is sent over the status IPC protocol. */
#include "alert_preview.h"
#include <string.h>
static size_t prefix(const char *s,size_t max){
 size_t n=strlen(s);if(n<=max)return n;n=max;if(!n)return 0;
 size_t start=n-1;while(start && ((unsigned char)s[start]&0xc0)==0x80)--start;
 unsigned char c=s[start];size_t need=c<0x80?1:c>=0xc2&&c<=0xdf?2:c>=0xe0&&c<=0xef?3:c>=0xf0&&c<=0xf4?4:1;
 return n-start<need?start:n;
}
static void add(char *out,size_t cap,const char *s,size_t limit,bool ellipsis){
 size_t used=strlen(out);if(used>=cap-1)return;size_t room=cap-used-1;size_t n=prefix(s,limit<room?limit:room);
 bool cut=s[n]!=0;size_t dots=cut && ellipsis && room>=3?3:0;if(dots && n+dots>room)n=prefix(s,room-dots);
 memcpy(out+used,s,n);used+=n;if(dots){memcpy(out+used,"...",3);used+=3;}out[used]=0;
}
void alert_preview(const AlertRoom *r,char *out,size_t cap){
 if(!cap)return;out[0]=0;
 const char *sender=r->sender[0]?r->sender:r->actor[0]?r->actor:"Someone";
 if(r->type!=1 && r->title[0]){add(out,cap,r->title,80,true);add(out,cap," / ",3,false);}
 add(out,cap,sender,72,true);add(out,cap,"\n",1,false);
 add(out,cap,r->body[0]?r->body:"Shared a message or attachment",240,true);
}
