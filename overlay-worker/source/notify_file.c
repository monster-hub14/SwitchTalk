#include "notify_file.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define mkdir(path,mode) _mkdir(path)
#endif
#ifndef NOTIFY_DIR
#define NOTIFY_DIR "sdmc:/config/ultrahand/notifications"
#endif
static bool ours(const char *s){size_t n=strlen(s);return n>17 && !strncmp(s,"switchtalk-",11) && !strcmp(s+n-7,".notify");}
static uint64_t instance;static unsigned sequence,write_errors;
static char last_real[256],last_test[256];
typedef struct {char token[128],path[256];long id;} Recent;
static Recent recent[8];static unsigned recent_slot;
void notify_begin(uint64_t value){instance=value;sequence=write_errors=recent_slot=0;last_real[0]=last_test[0]=0;memset(recent,0,sizeof(recent));}
static unsigned file_state(const char *path){if(!path[0])return 0;FILE *f=fopen(path,"rb");if(f){fclose(f);return 1;}return errno==ENOENT?2:3;}
void notify_diagnostics(unsigned *pending,unsigned *real,unsigned *test,unsigned *errors){
 *pending=0;DIR *d=opendir(NOTIFY_DIR);if(d){struct dirent *e;while((e=readdir(d)))if(ours(e->d_name))++*pending;closedir(d);}
 *real=file_state(last_real);*test=file_state(last_test);*errors=write_errors;
}
static bool json_text(FILE *f,const char *s){
 if(fputc('"',f)==EOF)return false;
 size_t n=strlen(s);if(n){size_t start=n-1;while(start && ((unsigned char)s[start]&0xc0)==0x80)--start;unsigned char c=s[start];size_t need=c<0x80?1:c>=0xc2&&c<=0xdf?2:c>=0xe0&&c<=0xef?3:c>=0xf0&&c<=0xf4?4:1;if(n-start<need)n=start;}
 for(const unsigned char *p=(const unsigned char*)s;p<(const unsigned char*)s+n;p++){
  if(*p=='"' || *p=='\\'){if(fputc('\\',f)==EOF)return false;}
  if(*p<32){if(fprintf(f,"\\u%04x",*p)<0)return false;}else if(fputc(*p,f)==EOF)return false;
 }
 return fputc('"',f)!=EOF;
}
bool notify_write(const char *token,long id,const char *text){
#ifndef ALERT_HOST
 mkdir("sdmc:/config",0777);mkdir("sdmc:/config/ultrahand",0777);
#endif
 mkdir(NOTIFY_DIR,0777);
 for(unsigned i=0;i<8;i++)if(recent[i].token[0] && recent[i].id==id && !strcmp(recent[i].token,token))return true;
 char path[256],temp[272];snprintf(path,sizeof(path),NOTIFY_DIR "/switchtalk-%016llx%08x.notify",(unsigned long long)instance,++sequence);
 DIR *d=opendir(NOTIFY_DIR);if(!d){++write_errors;return false;}unsigned count=0;struct dirent *e;while((e=readdir(d)))if(ours(e->d_name))++count;closedir(d);if(count>=8)return false;
 snprintf(temp,sizeof(temp),"%s.tmp",path);FILE *f=fopen(temp,"wb");if(!f){++write_errors;return false;}
 bool ok=fputs("{\"title\":\"SwitchTalk\",\"font_size\":22,\"priority\":20,\"duration\":5000,\"text\":",f)>=0 && json_text(f,text) && fputs("}\n",f)>=0;
 if(fflush(f))ok=false;if(fclose(f))ok=false;
 if(!ok){++write_errors;remove(temp);return false;}
 /* Verify the exact serialized bytes survive publication, before counting it. */
 unsigned char expected[2048],actual[2048];size_t expected_size=0,actual_size=0;
 f=fopen(temp,"rb");if(f){expected_size=fread(expected,1,sizeof(expected),f);if(fclose(f))expected_size=0;}
 if(!expected_size || expected_size==sizeof(expected) || rename(temp,path)){++write_errors;remove(temp);return false;}
 f=fopen(path,"rb");if(f){actual_size=fread(actual,1,sizeof(actual),f);if(fclose(f))actual_size=0;}
 /* Ultrahand may consume a file immediately. ENOENT after publication is not
    a readback error or proof of display. Keep the result explicitly unknown. */
 bool removed=!f && errno==ENOENT;
 if(!removed && (actual_size!=expected_size || memcmp(expected,actual,expected_size))){++write_errors;remove(path);return false;}
 Recent *entry=&recent[recent_slot++%8];snprintf(entry->token,sizeof(entry->token),"%s",token);entry->id=id;snprintf(entry->path,sizeof(entry->path),"%s",path);
 snprintf(!strcmp(token,"test")?last_test:last_real,sizeof(last_real),"%s",path);
 return true;
}
void notify_clear(void){memset(recent,0,sizeof(recent));recent_slot=0;last_real[0]=last_test[0]=0;DIR *d=opendir(NOTIFY_DIR);if(!d)return;struct dirent *e;while((e=readdir(d)))if(ours(e->d_name)){char p[256];snprintf(p,sizeof(p),NOTIFY_DIR "/%.150s",e->d_name);remove(p);}closedir(d);}
