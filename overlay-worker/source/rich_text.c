#include "rich_text.h"
#include <string.h>
#include <stdbool.h>
/* Remove an incomplete UTF-8 suffix if a bounded JSON string was truncated. */
static size_t complete_prefix(const char *text,size_t n){
 if(!n)return 0;size_t start=n-1;
 while(start && ((unsigned char)text[start]&0xc0)==0x80)--start;
 unsigned char lead=(unsigned char)text[start];size_t expected=lead<0x80?1:lead>=0xc2 && lead<=0xdf?2:lead>=0xe0 && lead<=0xef?3:lead>=0xf0 && lead<=0xf4?4:1;
 return n-start<expected?start:n;
}
static bool append(char *out,size_t capacity,size_t *used,const char *value,size_t n){
 size_t room=capacity-*used-1,want=n<room?n:room;
 want=complete_prefix(value,want);memcpy(out+*used,value,want);*used+=want;out[*used]=0;return want==n;
}
static bool key_char(unsigned char c){return (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='_' || c=='-' || c=='.';}
void talk_format_message(const JsonDoc *doc,int row,char *out,size_t capacity){
 if(!out || !capacity)return;out[0]=0;
 char message[1024],value[2048];json_string(doc,json_get(doc,row,"message"),message,sizeof(message));message[complete_prefix(message,strlen(message))]=0;
 int parameters=json_get(doc,row,"messageParameters");
 bool system=false;int event=json_get(doc,row,"systemMessage");
 if(event>=0 && doc->tokens[event].type==JSON_STRING && doc->tokens[event].end>doc->tokens[event].start)system=true;
 char type[32];json_string(doc,json_get(doc,row,"messageType"),type,sizeof(type));if(!strcmp(type,"system"))system=true;
 size_t used=0;const char *p=message;
 while(*p){
  if(*p=='{'){
   const char *end=p+1;while(key_char((unsigned char)*end))++end;size_t n=(size_t)(end-p-1);
   if(*end=='}' && n && n<128){
    char key[128];memcpy(key,p+1,n);key[n]=0;value[0]=0;
    int object=json_get(doc,parameters,key);
    if(object>=0 && doc->tokens[object].type==JSON_OBJECT){
     int name=json_get(doc,object,"name");
     if(name>=0 && doc->tokens[name].type==JSON_STRING)json_string(doc,name,value,sizeof(value));
     int id=json_get(doc,object,"id");
     if(!value[0] && id>=0 && (doc->tokens[id].type==JSON_STRING || (doc->tokens[id].type==JSON_PRIMITIVE && ((doc->text[doc->tokens[id].start]>='0' && doc->text[doc->tokens[id].start]<='9') || doc->text[doc->tokens[id].start]=='-'))))json_string(doc,id,value,sizeof(value));
    }
    /* Older/incomplete system events can omit actor parameters. This fallback
       never rewrites a literal {actor} in an ordinary user message. */
    int actor=json_get(doc,row,"actorDisplayName");
    if(!value[0] && system && !strcmp(key,"actor") && actor>=0 && doc->tokens[actor].type==JSON_STRING)json_string(doc,actor,value,sizeof(value));
    if(value[0]){
     size_t length=complete_prefix(value,strlen(value));if(!append(out,capacity,&used,value,length))break;
     p=end+1;continue;
    }
   }
  }
  /* Copy a literal run including unknown placeholders exactly as received. */
  const char *start=p++;while(*p && *p!='{')++p;
  if(!append(out,capacity,&used,start,(size_t)(p-start)))break;
 }
}
