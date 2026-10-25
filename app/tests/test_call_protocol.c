#include <stdlib.h>
#include "call_protocol.h"
#include "cJSON.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int main(void){
    const char *local="v=0\r\na=group:BUNDLE 2\r\nm=audio 9 UDP/TLS/RTP/SAVP 111\r\na=mid:2\r\na=rtpmap:111 opus/48000/2\r\n";
    const char *remote="v=0\r\na=group:BUNDLE 0 1\r\nm=audio 9 UDP/TLS/RTP/SAVPF 111\r\na=mid:0\r\na=rtpmap:111 opus/48000/2\r\nm=video 9 UDP/TLS/RTP/SAVPF 96 97\r\na=mid:1\r\n";
    char mid[64];char *s=call_sdp_answer(local,remote,mid,sizeof(mid));
    assert(s && !strcmp(mid,"0") && strstr(s,"a=group:BUNDLE 0\r\n") && strstr(s,"a=mid:0\r\n"));
    assert(strstr(s,"m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"));
    assert(strstr(s,"m=video 0 UDP/TLS/RTP/SAVPF 96 97\r\n") && strstr(s,"a=inactive"));free(s);
    assert(!call_sdp_answer(local,"v=0\r\nm=video 9 RTP/AVP 96\r\na=mid:1\r\n",mid,sizeof(mid)));
    cJSON *o=cJSON_CreateObject();cJSON_AddStringToObject(o,"sdp",remote);char *wire=cJSON_PrintUnformatted(o);cJSON *back=cJSON_Parse(wire);
    assert(!strcmp(cJSON_GetObjectItemCaseSensitive(back,"sdp")->valuestring,remote));cJSON_Delete(back);cJSON_Delete(o);cJSON_free(wire);
    puts("Call protocol: SFU audio mid, rejected video sections and exact SDP JSON round trip: PASS");return 0;
}
