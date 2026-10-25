#include "avatars.h"
#include "fixtures/avatar_fixtures.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
static int downloads,mode;static bool cancelled;
bool avatar_download(const TalkClient *t,const AvatarKey *key,unsigned char **data,size_t *size,AvatarCancel cancel,void *ctx){
 (void)t;(void)key;(void)cancel;(void)ctx;++downloads;
 if(mode==2)avatars_clear();
 if(mode==3){*data=malloc(3);memcpy(*data,"bad",3);*size=3;return true;}
 *size=sizeof(avatar_png);*data=malloc(*size);memcpy(*data,avatar_png,*size);return true;
}
static bool cancel(void *p){(void)p;return cancelled;}
int main(void){
 static uint32_t guarded[AVATAR_PIXELS+2];uint32_t *image=guarded+1;guarded[0]=0x12345678;guarded[AVATAR_PIXELS+1]=0x87654321;
 assert(avatar_decode(avatar_png,sizeof(avatar_png),image));assert(image[0]==0xffc5871a);
 assert(avatar_decode(avatar_jpg,sizeof(avatar_jpg),image));assert((image[0]>>24)==255);
 assert(!avatar_decode((const unsigned char*)"<svg/>",6,image));assert(!avatar_decode(avatar_png,AVATAR_DOWNLOAD_LIMIT+1,image));
 unsigned char png[sizeof(avatar_png)];memcpy(png,avatar_png,sizeof(png));png[16]=0;png[17]=0;png[18]=4;png[19]=1;assert(!avatar_decode(png,sizeof(png),image));
 assert(guarded[0]==0x12345678 && guarded[AVATAR_PIXELS+1]==0x87654321);
 avatars_init();static TalkClient t;
 AvatarKey key=avatars_user_key("real-user-id");assert(!avatars_get(&key,image));assert(avatars_pending());avatars_step(&t,cancel,NULL);assert(avatars_get(&key,image));assert(!avatars_pending() && downloads==1);unsigned revision=avatars_revision();
 avatars_get(&key,image);assert(!avatars_pending());
 TalkRoom room={.custom_avatar=true};strcpy(room.token,"room123");strcpy(room.avatar_version,"one");AvatarKey group=avatars_room_key(&room);assert(group.kind==AVATAR_ROOM);assert(!avatars_get(&group,image));avatars_step(&t,cancel,NULL);assert(avatars_get(&group,image));assert(avatars_revision()>revision);
 strcpy(room.avatar_version,"two");group=avatars_room_key(&room);assert(!avatars_get(&group,image));cancelled=true;avatars_step(&t,cancel,NULL);assert(avatars_pending());cancelled=false;avatars_step(&t,cancel,NULL);assert(avatars_get(&group,image));
 avatars_clear();assert(!avatars_get(&key,image));mode=2;avatars_step(&t,cancel,NULL);mode=0;assert(!avatars_get(&key,image)); /* in-flight old account result discarded */
 avatars_clear();mode=3;assert(!avatars_get(&key,image));avatars_step(&t,cancel,NULL);assert(!avatars_get(&key,image) && !avatars_pending());mode=0;
 for(int i=0;i<80;i++){char id[24];snprintf(id,sizeof(id),"user%d",i);AvatarKey many=avatars_user_key(id);avatars_get(&many,image);} /* bounded eviction */
 avatars_shutdown();puts("Avatars: actual PNG/JPEG decode, malformed/oversized bounds, identity/version cache, cancellation, account isolation, failed-image backoff and eviction: PASS");return 0;
}
