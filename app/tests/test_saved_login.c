#include "saved_login.h"
#include "server_url.h"
#include <switch.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
static int device=1,crypto_fail,rng_fail;static unsigned random_counter;
Result splCryptoInitialize(void){return crypto_fail?99:0;}void splCryptoExit(void){}
Result splCryptoGenerateAesKek(const void *source,uint32_t gen,uint32_t opt,void *out){assert(gen==4 && opt==1);for(int i=0;i<16;i++)((unsigned char*)out)[i]=((const unsigned char*)source)[i]^(unsigned char)device;return 0;}
Result splCryptoGenerateAesKey(const void *access,const void *salt,void *out){for(int i=0;i<16;i++)((unsigned char*)out)[i]=((const unsigned char*)access)[i]^((const unsigned char*)salt)[i];return 0;}
Result csrngInitialize(void){return rng_fail?99:0;}void csrngExit(void){}
Result csrngGetRandomBytes(void *p,size_t n){for(size_t i=0;i<n;i++)((unsigned char*)p)[i]=(unsigned char)++random_counter;return 0;}
static TalkClient account,loaded;
static unsigned char original[2048],changed[2048];static size_t size;
static void read_bytes(unsigned char *p){FILE *f=fopen(SAVED_LOGIN_DIR "/account.bin","rb");assert(f);size=fread(p,1,2048,f);fclose(f);assert(size>100);}
static void write_bytes(unsigned char *p,size_t n){FILE *f=fopen(SAVED_LOGIN_DIR "/account.bin","wb");assert(f);assert(fwrite(p,1,n,f)==n);assert(!fclose(f));}
static bool contains(const unsigned char *data,size_t n,const char *s){for(size_t i=0;i+strlen(s)<=n;i++)if(!memcmp(data+i,s,strlen(s)))return true;return false;}
int main(void){
 char url[512]="  cloud.example.com/nextcloud///  ";assert(server_normalize(url,sizeof(url)) && !strcmp(url,"https://cloud.example.com/nextcloud"));
 strcpy(url,"https://");assert(!server_has_host(url));strcpy(url,"http://bad.example");assert(!server_normalize(url,sizeof(url)));
 strcpy(url,"https://user@host/");assert(!server_normalize(url,sizeof(url)));strcpy(url,"https://example.com?foo=bar");assert(!server_normalize(url,sizeof(url)));
 strcpy(url,"https://https://host");assert(!server_normalize(url,sizeof(url)));strcpy(url,"HTTPS://Host.example:443/base/");assert(server_normalize(url,sizeof(url)) && !strcmp(url,"https://Host.example:443/base"));
 assert(saved_login_forget());assert(!saved_login_load(&loaded) && !saved_login_error()[0]);
 strcpy(account.server,"https://private.example/nextcloud");strcpy(account.username,"my-private-user");strcpy(account.app_password,"my-private-password");
 assert(saved_login_save(&account));read_bytes(original);assert(!contains(original,size,account.server) && !contains(original,size,account.username) && !contains(original,size,account.app_password));
 assert(saved_login_load(&loaded));assert(!strcmp(loaded.server,account.server) && !strcmp(loaded.username,account.username) && !strcmp(loaded.app_password,account.app_password));
 memset(&loaded,0,sizeof(loaded));assert(saved_login_load(&loaded)); /* fresh application state */
 device=2;strcpy(loaded.username,"unchanged");assert(!saved_login_load(&loaded));assert(!strcmp(loaded.username,"unchanged"));device=1;
 for(int i=0;i<4;i++){memcpy(changed,original,size);changed[i==0?10:i==1?19:i==2?35:60]^=1;write_bytes(changed,size);assert(!saved_login_load(&loaded));}
 write_bytes(original,size-1);assert(!saved_login_load(&loaded));write_bytes(original,size);
 crypto_fail=1;assert(!saved_login_save(&account));crypto_fail=0;assert(saved_login_load(&loaded));
 rng_fail=1;assert(!saved_login_save(&account));rng_fail=0;assert(saved_login_load(&loaded));
 assert(saved_login_save(&account));read_bytes(changed);assert(memcmp(original,changed,size));assert(saved_login_load(&loaded));
 assert(!rename(SAVED_LOGIN_DIR "/account.bin",SAVED_LOGIN_DIR "/account.bin.tmp"));assert(saved_login_load(&loaded));
 assert(saved_login_forget());assert(!saved_login_load(&loaded));
 puts("Saved sign-in: real AES-GCM, restart, no plaintext, console-key mismatch, tamper/truncation, RNG/crypto failure, temp recovery and sign-out erase: PASS");return 0;
}
