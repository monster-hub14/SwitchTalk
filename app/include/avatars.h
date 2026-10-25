#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "talk.h"
#define AVATAR_SIDE 64
#define AVATAR_PIXELS (AVATAR_SIDE*AVATAR_SIDE)
#define AVATAR_DOWNLOAD_LIMIT (512*1024)
typedef enum {AVATAR_NONE,AVATAR_USER,AVATAR_ROOM} AvatarKind;
typedef struct {AvatarKind kind;char id[128],version[64];} AvatarKey;
typedef bool (*AvatarCancel)(void*);
AvatarKey avatars_user_key(const char *id);
AvatarKey avatars_room_key(const TalkRoom *room);
void avatars_init(void);
void avatars_clear(void);
void avatars_shutdown(void);
/* Main-thread requests are deduplicated. Images are copied while locked. */
bool avatars_get(const AvatarKey *key,uint32_t out[AVATAR_PIXELS]);
unsigned avatars_revision(void);
bool avatars_pending(void);
/* Only the existing network worker calls step; no extra network thread. */
void avatars_step(const TalkClient *account,AvatarCancel cancel,void *context);
bool avatar_download(const TalkClient *account,const AvatarKey *key,unsigned char **data,size_t *size,AvatarCancel cancel,void *context);
bool avatar_decode(const unsigned char *data,size_t size,uint32_t out[AVATAR_PIXELS]);
