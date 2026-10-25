#include "qr_service.h"
#include <assert.h>
#include <string.h>
static QRReply rpc(unsigned op,unsigned arg,unsigned off,const char *token){QRRequest q={0};q.op=op;q.arg=arg;q.offset=off;if(token)strncpy(q.bytes,token,sizeof(q.bytes));QRReply r;qr_rpc(&q,&r);return r;}
static int delivered;
static bool filtered(const AlertRoom *room,void *ctx){(void)ctx;if(!qr_room_visible(room->token,0))++delivered;return true;}
int main(void){
 assert(sizeof(QRRequest)==144 && sizeof(QRReply)==176 && sizeof(QRReply)+40<=256);
 qr_init(123);QRReply r=rpc(QR_META,0,0,0);assert(!r.error && r.total==sizeof(QRMeta));
 AlertRoom rooms[2]={{.token="older",.title="Older",.id=1},{.token="newer",.title="Newer",.id=10}};qr_publish_rooms(rooms,2);
 QRRoom room={0};for(unsigned off=0;off<sizeof(room);off+=QR_DATA){r=rpc(QR_ROOM,0,off,0);assert(!r.error && r.total==sizeof(room));unsigned n=sizeof(room)-off;if(n>QR_DATA)n=QR_DATA;memcpy((char*)&room+off,r.bytes,n);}assert(!strcmp(room.token,"newer"));
 assert(rpc(QR_ROOM,2,0,0).error==QR_RANGE);assert(rpc(QR_ROOM,0,99999,0).error==QR_RANGE);
 QRRequest bad={.op=QR_OPEN};memset(bad.bytes,'x',sizeof(bad.bytes));qr_rpc(&bad,&r);assert(r.error==QR_BAD);
 assert(rpc(QR_OPEN,0,0,"unknown").error==QR_NOT_READY);
 r=rpc(QR_OPEN,0,0,"newer");assert(!r.error && qr_has_job());unsigned id=r.total;assert(rpc(QR_OPEN,0,0,"older").error==QR_BUSY);
 QRJob job;assert(qr_take_job(&job) && job.id==id && job.kind==QR_HISTORY_JOB && !strcmp(job.token,"newer"));assert(!qr_has_job() && !qr_take_job(&job));
 QRMessage message={.id=42,.sender="Alice",.body="Hello"};qr_complete_history(id,&message,1,0);
 QRMessage copy={0};for(unsigned off=0;off<sizeof(copy);off+=QR_DATA){r=rpc(QR_MESSAGE,0,off,0);assert(!r.error);unsigned n=sizeof(copy)-off;if(n>QR_DATA)n=QR_DATA;memcpy((char*)&copy+off,r.bytes,n);}assert(copy.id==42 && !strcmp(copy.body,"Hello"));
 r=rpc(QR_DRAFT_BEGIN,0,0,"newer");assert(!r.error);unsigned draft=r.total;
 QRRequest write={.op=QR_DRAFT_WRITE,.arg=draft,.size=5};memcpy(write.bytes,"Hello",5);qr_rpc(&write,&r);assert(!r.error);
 qr_rpc(&write,&r);assert(r.error==QR_BAD);write.offset=5;write.size=128;memset(write.bytes,'x',128);qr_rpc(&write,&r);assert(!r.error);
 r=rpc(QR_SEND,draft,0,0);assert(!r.error);unsigned sent_id=r.total;
 r=rpc(QR_SEND,draft,0,0);assert(!r.error && r.total==sent_id);
 assert(qr_take_job(&job) && job.kind==QR_SEND_JOB && strlen(job.text)==133);assert(!qr_take_job(&job));
 assert(rpc(QR_DRAFT_BEGIN,0,0,"older").error==QR_BUSY);
 qr_complete_send(sent_id,&message,0);r=rpc(QR_SEND,draft,0,0);assert(r.total==sent_id && r.state==QR_SENT && !qr_has_job());
 r=rpc(QR_DRAFT_BEGIN,0,0,"older");draft=r.total;write=(QRRequest){.op=QR_DRAFT_WRITE,.arg=draft,.size=2,.bytes="hi"};qr_rpc(&write,&r);assert(!r.error);
 r=rpc(QR_SEND,draft,0,0);sent_id=r.total;assert(qr_take_job(&job));qr_complete_send(sent_id,0,"HTTP 403");
 r=rpc(QR_DRAFT_READ,0,0,"older");assert(r.total==3 && !strcmp(r.bytes,"hi"));r=rpc(QR_SEND,draft,0,0);assert(r.state==QR_FAILED && !qr_has_job());
 r=rpc(QR_DRAFT_BEGIN,0,0,"older");draft=r.total;assert(rpc(QR_SEND,draft,0,0).error==QR_BAD);
 write=(QRRequest){.op=QR_DRAFT_WRITE,.arg=draft,.size=2};write.bytes[0]=(char)0xc0;write.bytes[1]=(char)0xaf;qr_rpc(&write,&r);assert(!r.error);assert(rpc(QR_SEND,draft,0,0).error==QR_BAD);
 r=rpc(QR_DRAFT_BEGIN,0,0,"older");draft=r.total;write=(QRRequest){.op=QR_DRAFT_WRITE,.arg=draft,.size=128};memset(write.bytes,'z',128);
 for(int i=0;i<4;i++){write.offset=i*128;qr_rpc(&write,&r);assert(!r.error);}write.offset=512;write.size=1;qr_rpc(&write,&r);assert(r.error==QR_BAD);

 r=rpc(QR_OPEN,0,0,"older");assert(!r.error);assert(qr_take_job(&job));qr_complete_history(job.id,0,0,"HTTP 403");assert(rpc(QR_META,0,0,0).state==QR_FAILED);
 assert(!qr_room_visible("newer",0));
 assert(!rpc(QR_VISIBLE,0,0,"newer").error);assert(qr_room_visible("newer",0));assert(!qr_room_visible("older",0));
 assert(qr_room_visible("newer",19200000ULL*5));assert(!qr_room_visible("newer",19200000ULL*6));
 assert(!rpc(QR_VISIBLE,1,0,"older").error);assert(qr_room_visible("older",0));assert(!qr_room_visible("newer",0));
 assert(!rpc(QR_VISIBLE,0,0,"").error);assert(!qr_room_visible("newer",0));assert(qr_room_visible("older",0));
 QRRequest covering={.op=QR_VISIBLE,.arg=1,.size=1};qr_rpc(&covering,&r);assert(!r.error && !qr_room_visible("older",0));
 assert(!rpc(QR_VISIBLE,1,0,"older").error);
 assert(rpc(QR_VISIBLE,2,0,"newer").error==QR_BAD);
 bad.op=QR_VISIBLE;bad.arg=0;qr_rpc(&bad,&r);assert(r.error==QR_BAD);
 qr_reset("Signed out");assert(!qr_room_visible("older",0));assert(!qr_has_job() && rpc(QR_META,0,0,0).state==QR_OFFLINE && rpc(QR_ROOM,0,0,0).error==QR_RANGE);
 AlertPolicy policy={0};rooms[0].id=100;rooms[1].id=200;
 alert_scan(&policy,rooms,2,"self","self",filtered,0);
 rpc(QR_VISIBLE,1,0,"older");rooms[0].id++;rooms[1].id++;
 alert_scan(&policy,rooms,2,"self","self",filtered,0);assert(delivered==1);
 rpc(QR_VISIBLE,1,0,"");alert_scan(&policy,rooms,2,"self","self",filtered,0);assert(delivered==1);
 rooms[0].id++;alert_scan(&policy,rooms,2,"self","self",filtered,0);assert(delivered==2);
 return 0;
}
