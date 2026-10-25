#include "call_protocol.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
static bool append(char *out,size_t *used,const char *fmt,...){
    va_list args;va_start(args,fmt);int n=vsnprintf(out+*used,32768-*used,fmt,args);va_end(args);
    if(n<0 || (size_t)n>=32768-*used)return false;*used+=(size_t)n;return true;
}
/* Align the audio mid and retain rejected media sections from an SFU offer. */
char *call_sdp_answer(const char *local,const char *remote,char *mid,size_t mid_cap){
    const char *audio=strstr(remote,"m=audio ");if(!audio || strstr(remote,"m=")!=audio)return NULL;
    const char *next=strstr(audio+2,"\r\nm="),*m=strstr(audio,"a=mid:");
    if(!m || (next && m>next))return NULL;m+=6;size_t len=strcspn(m,"\r\n");if(len>=mid_cap)return NULL;memcpy(mid,m,len);mid[len]=0;
    char protocol[64];int port,opus=-1;
    if(sscanf(audio,"m=audio %d %63s",&port,protocol)!=2)return NULL;
    for(const char *line=audio;(line=strstr(line,"a=rtpmap:"));line++){
        if(next && line>next)break;
        int pt;char codec[64];
        if(sscanf(line,"a=rtpmap:%d %63s",&pt,codec)==2 && !strcmp(codec,"opus/48000/2")){opus=pt;break;}
    }
    if(opus<0 || opus>127)return NULL;
    char *out=calloc(1,32768);if(!out)return NULL;
    size_t used=0;const char *p=local;
    while(*p){const char *end=strstr(p,"\r\n");size_t n=end?(size_t)(end-p):strlen(p);
        if(!strncmp(p,"a=group:BUNDLE",14)){if(!append(out,&used,"a=group:BUNDLE %s\r\n",mid))goto fail;}
        else if(!strncmp(p,"a=mid:",6)){if(!append(out,&used,"a=mid:%s\r\n",mid))goto fail;}
        else if(!strncmp(p,"m=audio ",8)){if(!append(out,&used,"m=audio 9 %s %d\r\n",protocol,opus))goto fail;}
        else if(!strncmp(p,"a=rtpmap:111 ",13)){if(!append(out,&used,"a=rtpmap:%d opus/48000/2\r\n",opus))goto fail;}
        else if(!strncmp(p,"a=sendrecv",10)){if(!append(out,&used,"a=recvonly\r\n"))goto fail;}
        else {if(used+n+3>=32768){free(out);return NULL;}memcpy(out+used,p,n);used+=n;memcpy(out+used,"\r\n",2);used+=2;}
        if(!end)break;p=end+2;
    }
    /* Janus normally puts audio first. Reject additional video/data m-lines. */
    for(const char *section=strstr(remote,"m=");section;){
        const char *end=strstr(section+2,"\r\nm=");
        if(section!=audio){
            const char *mid_line=strstr(section,"a=mid:");
            if(mid_line && (!end || mid_line<end)){
                char id[64]={0};size_t n=strcspn(mid_line+6,"\r\n");if(n>=sizeof(id)){free(out);return NULL;}memcpy(id,mid_line+6,n);
                const char *space=strchr(section,' '),*after_port=space?strchr(space+1,' '):NULL;
                const char *line_end=strstr(section,"\r\n");
                if(!space || !after_port || !line_end || after_port>line_end){free(out);return NULL;}
                if(!append(out,&used,"%.*s 0%.*s\r\nc=IN IP4 0.0.0.0\r\na=mid:%s\r\na=inactive\r\n",(int)(space-section),section,(int)(line_end-after_port),after_port,id))goto fail;
            }
        }
        section=end?end+2:NULL;
    }
    return out;
fail:
    free(out);return NULL;
}
