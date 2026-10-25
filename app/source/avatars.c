/* A bounded RAM-only cache; no pictures or account secrets are written to SD. */
#include "avatars.h"
#include <switch.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define CACHE_COUNT 48
typedef enum {EMPTY,QUEUED,LOADING,READY,FAILED} State;
typedef struct {AvatarKey key;uint32_t *image;State state;unsigned used;time_t retry;} Entry;
static Entry entries[CACHE_COUNT];static Mutex guard;static bool initialized;static unsigned generation,revision,clock_used;
AvatarKey avatars_user_key(const char *id){AvatarKey key={0};if(id && id[0]){key.kind=AVATAR_USER;snprintf(key.id,sizeof(key.id),"%s",id);}return key;}
AvatarKey avatars_room_key(const TalkRoom *room){
 if(room->avatar_user[0])return avatars_user_key(room->avatar_user);
 AvatarKey key={0};if(room->custom_avatar){key.kind=AVATAR_ROOM;snprintf(key.id,sizeof(key.id),"%s",room->token);snprintf(key.version,sizeof(key.version),"%s",room->avatar_version);}return key;
}
void avatars_init(void){if(!initialized){mutexInit(&guard);initialized=true;}}
void avatars_clear(void){
 if(!initialized)return;mutexLock(&guard);++generation;++revision;
 for(unsigned i=0;i<CACHE_COUNT;i++){free(entries[i].image);memset(&entries[i],0,sizeof(entries[i]));}
 mutexUnlock(&guard);
}
void avatars_shutdown(void){avatars_clear();}
static bool same(const AvatarKey *a,const AvatarKey *b){return a->kind==b->kind && !strcmp(a->id,b->id) && !strcmp(a->version,b->version);}
bool avatars_get(const AvatarKey *key,uint32_t out[AVATAR_PIXELS]){
 if(!initialized || !key || key->kind==AVATAR_NONE || !key->id[0] || !memchr(key->id,0,sizeof(key->id)) || !memchr(key->version,0,sizeof(key->version)))return false;
 mutexLock(&guard);int slot=-1,oldest=-1;unsigned age=~0u;
 for(int i=0;i<CACHE_COUNT;i++){
  Entry *e=&entries[i];if(e->state!=EMPTY && same(&e->key,key)){slot=i;break;}
  if(e->state!=LOADING && e->used<=age){oldest=i;age=e->used;}
 }
 if(slot<0 && oldest>=0){slot=oldest;Entry *e=&entries[slot];free(e->image);memset(e,0,sizeof(*e));e->key=*key;e->state=QUEUED;}
 bool found=false;
 if(slot>=0){Entry *e=&entries[slot];e->used=++clock_used;
  if((e->state==FAILED || e->state==READY) && time(NULL)>=e->retry)e->state=QUEUED;
  if(e->image){memcpy(out,e->image,AVATAR_PIXELS*sizeof(*out));found=true;}
 }
 mutexUnlock(&guard);return found;
}
unsigned avatars_revision(void){if(!initialized)return 0;mutexLock(&guard);unsigned value=revision;mutexUnlock(&guard);return value;}
bool avatars_pending(void){if(!initialized)return false;mutexLock(&guard);bool found=false;for(unsigned i=0;i<CACHE_COUNT;i++)if(entries[i].state==QUEUED){found=true;break;}mutexUnlock(&guard);return found;}
void avatars_step(const TalkClient *account,AvatarCancel cancel,void *context){
 if(!initialized)return;int slot=-1;unsigned gen;AvatarKey key={0};
 mutexLock(&guard);gen=generation;
 for(int i=0;i<CACHE_COUNT;i++)if(entries[i].state==QUEUED){slot=i;key=entries[i].key;entries[i].state=LOADING;break;}
 mutexUnlock(&guard);if(slot<0)return;
 unsigned char *data=NULL;size_t size=0;uint32_t *image=NULL;
 bool ok=avatar_download(account,&key,&data,&size,cancel,context);
 bool interrupted=cancel && cancel(context);
 if(ok && !interrupted){image=malloc(AVATAR_PIXELS*sizeof(*image));ok=image && avatar_decode(data,size,image);}else ok=false;
 free(data);
 mutexLock(&guard);
 if(gen==generation && same(&entries[slot].key,&key) && entries[slot].state==LOADING){
  Entry *e=&entries[slot];e->state=interrupted?QUEUED:(ok?READY:FAILED);e->retry=time(NULL)+600;
  if(ok){free(e->image);e->image=image;image=NULL;++revision;}
 }
 mutexUnlock(&guard);free(image);
}
