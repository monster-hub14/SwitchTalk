/* Console-bound AES-GCM login storage. No plaintext fallback or key file. */
#include "saved_login.h"
#include "server_url.h"
#include <switch.h>
#include <mbedtls/gcm.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <sys/stat.h>
#ifndef SAVED_LOGIN_DIR
#define SAVED_LOGIN_DIR "sdmc:/switch/SwitchTalk"
#endif
#define FILE_PATH SAVED_LOGIN_DIR "/account.bin"
#define TEMP_PATH SAVED_LOGIN_DIR "/account.bin.tmp"
typedef struct {char server[512],username[128],password[256];} Credentials;
typedef struct {unsigned char magic[8];uint32_t version,size;unsigned char salt[16],nonce[12],tag[16],ciphertext[sizeof(Credentials)];} Record;
typedef char StableEncryptedLoginHeader[(offsetof(Record,tag)==44)?1:-1];
static char last_error[160];
static void erase(void *p,size_t n){volatile unsigned char *b=p;while(n--)*b++=0;}
const char *saved_login_error(void){return last_error;}
/* Public domain-separation seed; the actual key is derived using this console's
   device master key, not this constant. Gen 4 selects the stable 4.0.0 generation
   shared by the supported Erista/Mariko firmware interfaces. Option bit 0 selects
   device-unique derivation. Regenerate the transient access key on each open. */
static bool derive(const unsigned char salt[16],unsigned char key[16]){
 static const unsigned char source[16]={0x53,0x77,0x69,0x74,0x63,0x68,0x54,0x61,0x6c,0x6b,0x4c,0x6f,0x67,0x69,0x6e,0x31};
 unsigned char access[16]={0};Result rc=splCryptoInitialize();
 if(R_FAILED(rc)){snprintf(last_error,sizeof(last_error),"Saved sign-in crypto unavailable (0x%08x)",rc);return false;}
 rc=splCryptoGenerateAesKek(source,4,1,access);
 if(!R_FAILED(rc))rc=splCryptoGenerateAesKey(access,salt,key);
 splCryptoExit();erase(access,sizeof(access));
 if(R_FAILED(rc)){erase(key,16);snprintf(last_error,sizeof(last_error),"Cannot unlock saved sign-in (0x%08x)",rc);return false;}return true;
}
static bool valid(const Credentials *c){
 return memchr(c->server,0,sizeof(c->server)) && memchr(c->username,0,sizeof(c->username)) && memchr(c->password,0,sizeof(c->password)) && !strncmp(c->server,"https://",8) && server_has_host(c->server) && c->username[0] && c->password[0];
}
static bool read_record(const char *path,Credentials *out){
 Record record={0};unsigned char key[16]={0};bool ok=false;FILE *f=fopen(path,"rb");if(!f)return false;
 bool complete=fread(&record,1,sizeof(record),f)==sizeof(record) && fgetc(f)==EOF;fclose(f);
 if(!complete || memcmp(record.magic,"SWTLOGIN",8) || record.version!=1 || record.size!=sizeof(*out)){snprintf(last_error,sizeof(last_error),"Saved sign-in file is invalid; sign in again");goto done;}
 if(!derive(record.salt,key))goto done;
 mbedtls_gcm_context gcm;mbedtls_gcm_init(&gcm);
 int rc=mbedtls_gcm_setkey(&gcm,MBEDTLS_CIPHER_ID_AES,key,128);
 if(!rc)rc=mbedtls_gcm_auth_decrypt(&gcm,sizeof(*out),record.nonce,sizeof(record.nonce),(unsigned char*)&record,offsetof(Record,tag),record.tag,sizeof(record.tag),record.ciphertext,(unsigned char*)out);
 mbedtls_gcm_free(&gcm);
 ok=!rc && valid(out);if(!ok)snprintf(last_error,sizeof(last_error),"Saved sign-in could not be unlocked on this Switch; sign in again");
 done:erase(key,sizeof(key));erase(&record,sizeof(record));if(!ok)erase(out,sizeof(*out));return ok;
}
bool saved_login_load(TalkClient *client){
 last_error[0]=0;Credentials c={0};bool ok=read_record(FILE_PATH,&c);
 /* A completed temporary record is recoverable if power was lost during rename. */
 if(!ok)ok=read_record(TEMP_PATH,&c);
 if(ok){snprintf(client->server,sizeof(client->server),"%s",c.server);snprintf(client->username,sizeof(client->username),"%s",c.username);snprintf(client->app_password,sizeof(client->app_password),"%s",c.password);last_error[0]=0;}
 erase(&c,sizeof(c));return ok;
}
bool saved_login_save(const TalkClient *client){
 Credentials c={0};Record record={.magic={'S','W','T','L','O','G','I','N'},.version=1,.size=sizeof(Credentials)};unsigned char key[16]={0};bool ok=false;
 last_error[0]=0;snprintf(c.server,sizeof(c.server),"%s",client->server);snprintf(c.username,sizeof(c.username),"%s",client->username);snprintf(c.password,sizeof(c.password),"%s",client->app_password);
 if(!valid(&c)){snprintf(last_error,sizeof(last_error),"Sign-in is incomplete; nothing saved");goto done;}
 Result random=csrngInitialize();if(R_FAILED(random)){snprintf(last_error,sizeof(last_error),"Saved sign-in random source unavailable");goto done;}
 random=csrngGetRandomBytes(record.salt,sizeof(record.salt));if(!R_FAILED(random))random=csrngGetRandomBytes(record.nonce,sizeof(record.nonce));csrngExit();
 if(R_FAILED(random)){snprintf(last_error,sizeof(last_error),"Cannot generate saved sign-in nonce");goto done;}
 if(!derive(record.salt,key))goto done;
 mbedtls_gcm_context gcm;mbedtls_gcm_init(&gcm);int rc=mbedtls_gcm_setkey(&gcm,MBEDTLS_CIPHER_ID_AES,key,128);
 if(!rc)rc=mbedtls_gcm_crypt_and_tag(&gcm,MBEDTLS_GCM_ENCRYPT,sizeof(c),record.nonce,sizeof(record.nonce),(unsigned char*)&record,offsetof(Record,tag),(unsigned char*)&c,record.ciphertext,sizeof(record.tag),record.tag);
 mbedtls_gcm_free(&gcm);if(rc){snprintf(last_error,sizeof(last_error),"Could not encrypt saved sign-in");goto done;}
#ifndef SAVED_LOGIN_HOST
 mkdir("sdmc:/switch",0777);mkdir(SAVED_LOGIN_DIR,0700);
#endif
 FILE *f=fopen(TEMP_PATH,"wb");if(!f){snprintf(last_error,sizeof(last_error),"Cannot write saved sign-in to SD card");goto done;}
 bool wrote=fwrite(&record,1,sizeof(record),f)==sizeof(record);if(fflush(f))wrote=false;if(fclose(f))wrote=false;
 if(!wrote){remove(TEMP_PATH);snprintf(last_error,sizeof(last_error),"SD card write failed; sign-in was not saved");goto done;}
 /* FAT rename may not replace an existing target. Keep the complete temp file
    until the rename succeeds, so a power interruption can be recovered above. */
 if(rename(TEMP_PATH,FILE_PATH)){
  if(remove(FILE_PATH) && errno!=ENOENT){snprintf(last_error,sizeof(last_error),"Could not replace saved sign-in");goto done;}
  if(rename(TEMP_PATH,FILE_PATH)){snprintf(last_error,sizeof(last_error),"Could not finalize saved sign-in");goto done;}
 }
 ok=true;
 done:erase(&c,sizeof(c));erase(&record,sizeof(record));erase(key,sizeof(key));return ok;
}
bool saved_login_forget(void){
 last_error[0]=0;bool ok=true;if(remove(FILE_PATH) && errno!=ENOENT)ok=false;if(remove(TEMP_PATH) && errno!=ENOENT)ok=false;
 if(!ok)snprintf(last_error,sizeof(last_error),"Could not remove saved sign-in from SD card");return ok;
}

