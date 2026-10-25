#include "rich_text.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static JsonDoc doc;
static void check(const char *json,const char *expected){char out[1024];assert(!json_parse(&doc,json));talk_format_message(&doc,0,out,sizeof(out));if(strcmp(out,expected)){fprintf(stderr,"Expected: [%s] actual: [%s]\n",expected,out);assert(0);}}
int main(void){
 check("{\"message\":\"{actor} left the call\",\"messageParameters\":{\"actor\":{\"type\":\"user\",\"id\":\"zayd-id\",\"name\":\"zayd\"}}}","zayd left the call");
 check("{\"message\":\"Call with {user1} and {user2} ended\",\"messageParameters\":{\"user1\":{\"name\":\"zayd\"},\"user2\":{\"name\":\"Alex\"}}}","Call with zayd and Alex ended");
 check("{\"message\":\"{actor} renamed {file-1} to {name}\",\"messageParameters\":{\"actor\":{\"name\":\"Alex\"},\"file-1\":{\"name\":\"Photo.jpg\"},\"name\":{\"name\":\"New photo.jpg\"}}}","Alex renamed Photo.jpg to New photo.jpg");
 check("{\"message\":\"Hello {user1}, {user1}!\",\"messageParameters\":{\"user1\":{\"name\":\"Alex\"}}}","Hello Alex, Alex!");
 check("{\"message\":\"{user} / {user1}\",\"messageParameters\":{\"user\":{\"name\":\"Sam\"},\"user1\":{\"name\":\"Alex\"}}}","Sam / Alex");
 check("{\"message\":\"literal {actor} and {code}\",\"actorDisplayName\":\"zayd\",\"systemMessage\":\"\",\"messageType\":\"comment\",\"messageParameters\":[]}","literal {actor} and {code}");
 check("{\"message\":\"{actor} left\",\"actorDisplayName\":\"zayd\",\"systemMessage\":\"call_left\"}","zayd left");
 check("{\"message\":\"{actor} joined\",\"actorDisplayName\":\"zayd\",\"messageType\":\"system\",\"messageParameters\":null}","zayd joined");
 check("{\"message\":\"{user1} left\",\"messageParameters\":{\"user1\":{\"name\":\"\",\"id\":\"deleted-user\"}}}","deleted-user left");
 check("{\"message\":\"{user1} left\",\"messageParameters\":{\"user1\":{\"name\":null,\"id\":\"guest-1\"}}}","guest-1 left");
 check("{\"message\":\"{missing} {bad} {} {unfinished\",\"messageParameters\":{\"bad\":{\"name\":null,\"id\":null}}}","{missing} {bad} {} {unfinished");
 check("{\"message\":\"{actor} left\",\"messageParameters\":{\"actor\":{\"name\":\"{actor}\"}}}","{actor} left");
 check("{\"message\":\"Hi {person}\",\"messageParameters\":{\"person\":{\"name\":\"Jos\\u00e9 \\ud83d\\ude00\"}}}","Hi Jos\xc3\xa9 \xf0\x9f\x98\x80");
 check("{\"message\":\"{actor}\",\"messageParameters\":{\"actor\":{\"name\":\"<Admin>& \\\"Name\\\"\\u001b[31m\"}}}","<Admin>& \"Name\"[31m");
 assert(!json_parse(&doc,"{\"message\":\"{actor} left\",\"messageParameters\":{\"actor\":{\"name\":\"zayd\"}}}"));
 char small[10];memset(small,'X',sizeof(small));talk_format_message(&doc,0,small,1);assert(!small[0] && small[1]=='X');talk_format_message(&doc,0,small,0);assert(small[1]=='X');talk_format_message(&doc,0,small,6);assert(!strcmp(small,"zayd ") && small[6]=='X');
 assert(!json_parse(&doc,"{\"message\":\"{actor}!\",\"messageParameters\":{\"actor\":{\"name\":\"A\\ud83d\\ude00B\"}}}"));
 talk_format_message(&doc,0,small,4);assert(!strcmp(small,"A"));talk_format_message(&doc,0,small,6);assert(!strcmp(small,"A\xf0\x9f\x98\x80"));
 char large[4096];strcpy(large,"{\"message\":\"{actor}\",\"messageParameters\":{\"actor\":{\"name\":\"");size_t at=strlen(large);memset(large+at,'N',2500);strcpy(large+at+2500,"\"}}}");assert(!json_parse(&doc,large));char output[1025];memset(output,'X',sizeof(output));talk_format_message(&doc,0,output,1024);assert(strlen(output)==1023 && output[1024]=='X');
 puts("Rich messages: call participants, generic parameters, repeated/exact keys, plain text, actor fallback, missing/null metadata, nonrecursive replacement and UTF-8/buffer bounds: PASS");return 0;
}


