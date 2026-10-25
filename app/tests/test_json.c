#include "json.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static JsonDoc d;
int main(void){
    const char *test="{\"ocs\":{\"meta\":{\"statuscode\":100},\"data\":[{\"token\":\"abc123\",\"displayName\":\"Rai's room\",\"unreadMessages\":2},{\"displayName\":\"Zaki \\uD83D\\uDE80\"}]}}";
    assert(json_parse(&d,test)==0);
    int ocs=json_get(&d,0,"ocs");assert(ocs>=0);
    int data=json_get(&d,ocs,"data");assert(json_count(&d,data)==2);
    char room[80];json_string(&d,json_get(&d,json_item(&d,data,0),"displayName"),room,sizeof(room));
    assert(!strcmp(room,"Rai's room"));
    int c=json_get(&d,json_item(&d,data,0),"unreadMessages");assert(json_long(&d,c)==2);
    json_string(&d,json_get(&d,json_item(&d,data,1),"displayName"),room,sizeof(room));
    assert(strstr(room,"Zaki ")!=NULL);
    const char *encoded="{\"data\": [{\"message\":\"hello\\nworld\\u0021\", \"actorDisplayName\":\"Zaki\"}]}";
    assert(json_parse(&d,encoded)==0);
    int msg=json_item(&d,json_get(&d,0,"data"),0);
    char v[80];json_string(&d,json_get(&d,msg,"message"),v,sizeof(v));
    assert(!strcmp(v,"hello world!"));
    assert(json_parse(&d,"{\"broken\": [1,2,]}")<0);
    assert(json_parse(&d,"\"unterminated")<0);
    assert(json_parse(&d,"[1,2] extra")<0);
    puts("JSON parser tests: PASSED");
    return 0;
}
