#include "qr_service.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef QR_HOST
#include <switch.h>
static Mutex guard;
#define LOCK() mutexLock(&guard)
#define UNLOCK() mutexUnlock(&guard)
#else
#define LOCK() ((void)0)
#define UNLOCK() ((void)0)
#endif
static QRMeta meta;static QRRoom rooms[ALERT_ROOMS];static QRMessage messages[QR_MESSAGES];
static QRJob job;static bool queued,busy;static uint32_t sequence,revision,draft_id,last_send_draft,last_send_id,last_send_state;
static char draft_token[128],draft[QR_TEXT+1];static size_t draft_size;
/* Two independent viewers: full application and overlay. No network or extra thread. */
static struct {char token[128];uint64_t stamp;bool active;} viewers[2];
#define VIEW_LEASE (UINT64_C(19200000)*6)
#ifndef QR_HOST
#define VIEW_NOW() svcGetSystemTick()
#else
#define VIEW_NOW() UINT64_C(0)
#endif
bool qr_room_visible(const char *token,uint64_t now){LOCK();bool yes=false;
 /* A visible overlay covers the app, including its conversation-list screen. */
 int slot=(viewers[1].active && now-viewers[1].stamp<VIEW_LEASE)?1:0;
 if(viewers[slot].active && now-viewers[slot].stamp<VIEW_LEASE && token[0] && !strcmp(token,viewers[slot].token))yes=true;
 UNLOCK();return yes;}
static void wipe(void *p,size_t n){volatile unsigned char *b=p;while(n--)*b++=0;}
static void text(char *out,size_t cap,const char *in){snprintf(out,cap,"%s",in?in:"");size_t n=strlen(out);if(!n)return;size_t s=n-1;while(s && ((unsigned char)out[s]&0xc0)==0x80)--s;unsigned char c=out[s];size_t need=c<128?1:c>=0xc2&&c<=0xdf?2:c>=0xe0&&c<=0xef?3:c>=0xf0&&c<=0xf4?4:1;if(n-s<need)out[s]=0;}
static bool utf8(const char *s,size_t n){for(size_t i=0;i<n;){unsigned char c=s[i];if(c<0x80){if(!c || (c<32 && c!='\n' && c!='\t'))return false;++i;continue;}unsigned k=c>=0xc2&&c<=0xdf?2:c>=0xe0&&c<=0xef?3:c>=0xf0&&c<=0xf4?4:0;if(!k || i+k>n)return false;uint32_t cp=c&((1u<<(7-k))-1);for(unsigned j=1;j<k;j++){unsigned char b=s[i+j];if((b&0xc0)!=0x80)return false;cp=(cp<<6)|(b&63);}if((k==3&&cp<0x800)||(k==4&&cp<0x10000)||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))return false;i+=k;}return true;}
static bool known(const char *token){for(unsigned i=0;i<meta.rooms;i++)if(!strcmp(rooms[i].token,token))return true;return false;}
static void copy_out(const QRRequest *q,QRReply *out,const void *data,size_t size){out->total=size;if(q->offset>size){out->error=QR_RANGE;return;}size_t n=size-q->offset;if(n>sizeof(out->bytes))n=sizeof(out->bytes);memcpy(out->bytes,(const char*)data+q->offset,n);}
void qr_init(uint64_t instance){
#ifndef QR_HOST
 mutexInit(&guard);
#endif
 memset(viewers,0,sizeof(viewers));memset(&meta,0,sizeof(meta));queued=busy=false;draft_id=last_send_draft=last_send_id=last_send_state=0;draft_size=0;wipe(&job,sizeof(job));wipe(draft,sizeof(draft));wipe(draft_token,sizeof(draft_token));meta.instance=instance;meta.state=QR_OFFLINE;revision=1;sequence=0;text(meta.notice,sizeof(meta.notice),"Start worker and sign in using SwitchTalk");}
void qr_reset(const char *notice){LOCK();memset(viewers,0,sizeof(viewers));meta.rooms=meta.messages=0;meta.token[0]=0;meta.state=QR_OFFLINE;meta.job_id=0;text(meta.notice,sizeof(meta.notice),notice);queued=busy=false;draft_size=0;draft_id=0;last_send_draft=0;wipe(draft,sizeof(draft));wipe(draft_token,sizeof(draft_token));wipe(messages,sizeof(messages));wipe(rooms,sizeof(rooms));wipe(&job,sizeof(job));++revision;UNLOCK();}
static int compare(const void *a,const void *b){const QRRoom *x=a,*y=b;return x->last_id<y->last_id?1:x->last_id>y->last_id?-1:strcmp(x->title,y->title);}
void qr_publish_rooms(const AlertRoom *input,int count){LOCK();if(count<0)count=0;if(count>ALERT_ROOMS)count=ALERT_ROOMS;meta.rooms=count;
 for(int i=0;i<count;i++){QRRoom *r=&rooms[i];memset(r,0,sizeof(*r));text(r->token,sizeof(r->token),input[i].token);text(r->title,sizeof(r->title),input[i].title);text(r->preview,sizeof(r->preview),input[i].body);r->last_id=input[i].id;}
 qsort(rooms,count,sizeof(*rooms),compare);if(meta.state==QR_OFFLINE){meta.state=QR_IDLE;text(meta.notice,sizeof(meta.notice),"Select a conversation");}++revision;UNLOCK();}
void qr_rpc(const QRRequest *q,QRReply *out){memset(out,0,sizeof(*out));LOCK();out->revision=revision;out->state=meta.state;
#if !QR_ENABLE_SEND
 if(q->op>=QR_DRAFT_BEGIN && q->op<=QR_DRAFT_READ){out->error=QR_NOT_READY;UNLOCK();return;}
#endif
 switch(q->op){
 case QR_VISIBLE:
  if(q->arg>=2 || q->size>1 || !memchr(q->bytes,0,sizeof(q->bytes))){out->error=QR_BAD;break;}
  text(viewers[q->arg].token,sizeof(viewers[q->arg].token),q->bytes);
  viewers[q->arg].stamp=VIEW_NOW();viewers[q->arg].active=q->bytes[0]!=0 || q->size==1;break;

 case QR_META:copy_out(q,out,&meta,sizeof(meta));break;
 case QR_ROOM:if(q->arg>=meta.rooms)out->error=QR_RANGE;else copy_out(q,out,&rooms[q->arg],sizeof(QRRoom));break;
 case QR_MESSAGE:if(q->arg>=meta.messages)out->error=QR_RANGE;else copy_out(q,out,&messages[q->arg],sizeof(QRMessage));break;
 case QR_OPEN:
  if(!memchr(q->bytes,0,sizeof(q->bytes)) || !q->bytes[0]){out->error=QR_BAD;break;}
  if(busy){out->error=QR_BUSY;break;}if(!known(q->bytes)){out->error=QR_NOT_READY;break;}
  if(strcmp(meta.token,q->bytes)){meta.messages=0;wipe(messages,sizeof(messages));}text(meta.token,sizeof(meta.token),q->bytes);
  memset(&job,0,sizeof(job));job.id=++sequence;job.kind=QR_HISTORY_JOB;text(job.token,sizeof(job.token),q->bytes);queued=busy=true;meta.job_id=job.id;meta.state=QR_PENDING;text(meta.notice,sizeof(meta.notice),"Loading recent messages...");out->total=job.id;++revision;break;
 case QR_DRAFT_BEGIN:
  if(busy){out->error=QR_BUSY;break;}if(!memchr(q->bytes,0,sizeof(q->bytes)) || !known(q->bytes)){out->error=QR_NOT_READY;break;}
  draft_id=++sequence;draft_size=0;wipe(draft,sizeof(draft));text(draft_token,sizeof(draft_token),q->bytes);out->total=draft_id;break;
 case QR_DRAFT_WRITE:
  if(q->arg!=draft_id || !draft_id){out->error=QR_STALE;break;}if(busy){out->error=QR_BUSY;break;}
  if(q->size>sizeof(q->bytes) || q->offset!=draft_size || q->size>QR_TEXT-draft_size || memchr(q->bytes,0,q->size)){out->error=QR_BAD;break;}
  memcpy(draft+draft_size,q->bytes,q->size);draft_size+=q->size;draft[draft_size]=0;out->total=draft_size;break;
 case QR_SEND:
  if(q->arg && q->arg==last_send_draft){out->total=last_send_id;out->state=last_send_state;break;}
  if(q->arg!=draft_id || !draft_id){out->error=QR_STALE;break;}if(busy){out->error=QR_BUSY;break;}
  if(!draft_size || !utf8(draft,draft_size) || !known(draft_token)){out->error=QR_BAD;break;}
  memset(&job,0,sizeof(job));job.id=++sequence;job.kind=QR_SEND_JOB;text(job.token,sizeof(job.token),draft_token);text(job.text,sizeof(job.text),draft);
  if(strcmp(meta.token,draft_token)){meta.messages=0;wipe(messages,sizeof(messages));}
  text(meta.token,sizeof(meta.token),draft_token);meta.job_id=job.id;meta.state=QR_PENDING;text(meta.notice,sizeof(meta.notice),"Sending...");queued=busy=true;
  last_send_draft=draft_id;last_send_id=job.id;last_send_state=QR_PENDING;out->total=job.id;out->state=QR_PENDING;++revision;break;
 case QR_DRAFT_READ:
  if(!memchr(q->bytes,0,sizeof(q->bytes))){out->error=QR_BAD;break;}
  if(!strcmp(q->bytes,draft_token))copy_out(q,out,draft,draft_size+1);else copy_out(q,out,"",1);break;
 default:out->error=QR_BAD;
 }
 UNLOCK();}
bool qr_has_job(void){LOCK();bool yes=queued;UNLOCK();return yes;}
bool qr_take_job(QRJob *out){LOCK();bool yes=queued;if(yes){*out=job;queued=false;}UNLOCK();return yes;}
void qr_complete_history(uint32_t id,const QRMessage *input,int count,const char *error){LOCK();if(!busy || meta.job_id!=id){UNLOCK();return;}busy=false;meta.state=error?QR_FAILED:QR_READY;if(error){text(meta.notice,sizeof(meta.notice),error);}else{if(count<0)count=0;if(count>QR_MESSAGES)count=QR_MESSAGES;meta.messages=count;if(count)memcpy(messages,input,count*sizeof(*input));text(meta.notice,sizeof(meta.notice),count?"Recent messages loaded":"No messages yet");}wipe(&job,sizeof(job));++revision;UNLOCK();}
void qr_complete_send(uint32_t id,const QRMessage *sent,const char *error){LOCK();if(!busy || meta.job_id!=id){UNLOCK();return;}busy=false;meta.state=error?QR_FAILED:QR_SENT;last_send_state=meta.state;text(meta.notice,sizeof(meta.notice),error?error:"Sent");
 if(!error){if(meta.messages==QR_MESSAGES){memmove(messages,messages+1,(QR_MESSAGES-1)*sizeof(*messages));--meta.messages;}if(sent)messages[meta.messages++]=*sent;wipe(draft,sizeof(draft));draft_size=0;}
 wipe(&job,sizeof(job));++revision;UNLOCK();}
