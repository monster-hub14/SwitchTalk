/* Only PNG/JPEG avatars are decoded. Enforce encoded, dimension and heap limits. */
#include "avatars.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define IMAGE_HEAP_LIMIT (16*1024*1024)
typedef struct {size_t size,pad;} ImageAllocation;
static size_t image_allocated;
static void *image_malloc(size_t size){
 if(size>IMAGE_HEAP_LIMIT-image_allocated || size>SIZE_MAX-sizeof(ImageAllocation))return NULL;
 ImageAllocation *p=malloc(size+sizeof(*p));if(!p)return NULL;p->size=size;image_allocated+=size;return p+1;
}
static void image_free(void *p){if(p){ImageAllocation *header=(ImageAllocation*)p-1;image_allocated-=header->size;free(header);}}
static void *image_realloc(void *p,size_t size){
 if(!p)return image_malloc(size);ImageAllocation *h=(ImageAllocation*)p-1;size_t old=h->size;
 if(size>IMAGE_HEAP_LIMIT-(image_allocated-old) || size>SIZE_MAX-sizeof(*h))return NULL;
 ImageAllocation *q=realloc(h,size+sizeof(*h));if(!q)return NULL;q->size=size;image_allocated=image_allocated-old+size;return q+1;
}
#define STBI_MALLOC(n) image_malloc(n)
#define STBI_REALLOC(p,n) image_realloc(p,n)
#define STBI_FREE(p) image_free(p)
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_MAX_DIMENSIONS 1024
#define STB_IMAGE_IMPLEMENTATION
#include "../vendor/stb/stb_image.h"
bool avatar_decode(const unsigned char *data,size_t size,uint32_t out[AVATAR_PIXELS]){
 int width,height,channels;if(!data || !size || size>AVATAR_DOWNLOAD_LIMIT)return false;
 if(!stbi_info_from_memory(data,(int)size,&width,&height,&channels) || width<1 || height<1 || width>1024 || height>1024)return false;
 unsigned char *rgba=stbi_load_from_memory(data,(int)size,&width,&height,&channels,4);if(!rgba)return false;
 for(int y=0;y<AVATAR_SIDE;y++)for(int x=0;x<AVATAR_SIDE;x++){
  /* Center crop rectangular images; server avatars are normally square. */
  int side=width<height?width:height;
  int sx=(width-side)/2+x*side/AVATAR_SIDE,sy=(height-side)/2+y*side/AVATAR_SIDE;
  const unsigned char *p=rgba+(sy*width+sx)*4;
  out[y*AVATAR_SIDE+x]=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
 }
 stbi_image_free(rgba);return true;
}
