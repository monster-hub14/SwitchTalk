#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "call_http.h"
#include "tls.h"
#include <mbedtls/base64.h>
#include <mbedtls/sha1.h>
static CURL *ws;
static unsigned char receive[131073],message[131072];
static size_t receive_len,message_len;
static bool failed;
static int cancel(void *ctx,curl_off_t a,curl_off_t b,curl_off_t c,curl_off_t d){(void)ctx;(void)a;(void)b;(void)c;(void)d;return call_cancelled();}
static bool send_bytes(const unsigned char *buf,size_t len){
    u64 start=armGetSystemTick();
    while(len && !call_cancelled()){
        size_t n=0;CURLcode rc=curl_easy_send(ws,buf,len,&n);
        if(rc!=CURLE_OK && rc!=CURLE_AGAIN)return false;
        buf+=n;len-=n;
        if(armTicksToNs(armGetSystemTick()-start)>5000000000ULL)return false;
        if(!n)svcSleepThread(2000000);
    }
    return !len;
}
static bool frame(unsigned opcode,const void *payload,size_t len){
    if(!ws || len>65535)return false;
    unsigned char *buf=malloc(len+8);if(!buf)return false;
    size_t off=2;buf[0]=0x80|opcode;
    if(len<126)buf[1]=0x80|len;
    else {buf[1]=0x80|126;buf[2]=(len>>8)&255;buf[3]=len&255;off=4;}
    randomGet(buf+off,4);off+=4;
    for(size_t i=0;i<len;i++)buf[off+i]=((const unsigned char*)payload)[i]^buf[off-4+i%4];
    bool ok=send_bytes(buf,len+off);free(buf);if(!ok)failed=true;return ok;
}
static int fill(void){
    if(receive_len>=sizeof(receive)-1)return -1;
    size_t n=0;CURLcode rc=curl_easy_recv(ws,receive+receive_len,sizeof(receive)-1-receive_len,&n);
    if(rc==CURLE_AGAIN)return 0;
    if(rc!=CURLE_OK || !n)return -1;
    receive_len+=n;return 1;
}
bool call_ws_open(const char *server,const char *origin,char *error,size_t cap){
    receive_len=message_len=0;failed=false;
    char uri[2048];const char *rest;
    if(!strncmp(server,"wss://",6))rest=server+6;
    else if(!strncmp(server,"https://",8))rest=server+8;
    else {snprintf(error,cap,"Signaling requires a secure wss/https URL");return false;}
    if(strlen(rest)>1900){snprintf(error,cap,"Signaling URL too long");return false;}
    snprintf(uri,sizeof(uri),"https://%s",rest);while(uri[strlen(uri)-1]=='/')uri[strlen(uri)-1]=0;
    if(strlen(uri)<7 || strcmp(uri+strlen(uri)-7,"/spreed"))strcat(uri,"/spreed");
    CURLU *u=curl_url();char *host=NULL,*port=NULL,*path=NULL;
    if(!u || curl_url_set(u,CURLUPART_URL,uri,0)!=CURLUE_OK){if(u)curl_url_cleanup(u);snprintf(error,cap,"Invalid signaling URL");return false;}
    curl_url_get(u,CURLUPART_HOST,&host,0);curl_url_get(u,CURLUPART_PORT,&port,0);curl_url_get(u,CURLUPART_PATH,&path,0);
    unsigned char nonce[16],key[32],sha[20],accept[40];size_t written;
    randomGet(nonce,16);mbedtls_base64_encode(key,sizeof(key),&written,nonce,16);key[written]=0;
    char source[96];snprintf(source,sizeof(source),"%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11",key);mbedtls_sha1_ret((unsigned char*)source,strlen(source),sha);mbedtls_base64_encode(accept,sizeof(accept),&written,sha,20);accept[written]=0;
    char request[4096];snprintf(request,sizeof(request),"GET %s HTTP/1.1\r\nHost: %s%s%s\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: %s\r\nOrigin: %s\r\n\r\n",path?path:"/spreed",host?host:"",port?":":"",port?port:"",key,origin);
    curl_free(host);curl_free(port);curl_free(path);curl_url_cleanup(u);
    ws=curl_easy_init();if(!ws){snprintf(error,cap,"Cannot initialize signaling socket");return false;}
    curl_easy_setopt(ws,CURLOPT_URL,uri);curl_easy_setopt(ws,CURLOPT_CONNECT_ONLY,1L);curl_easy_setopt(ws,CURLOPT_CONNECTTIMEOUT,10L);curl_easy_setopt(ws,CURLOPT_TIMEOUT,15L);
    curl_easy_setopt(ws,CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(ws,CURLOPT_SSL_VERIFYHOST,2L);if(talk_tls_configure(ws)!=CURLE_OK){snprintf(error,cap,"Cannot configure signaling TLS trust");return false;}
    curl_easy_setopt(ws,CURLOPT_NOPROGRESS,0L);curl_easy_setopt(ws,CURLOPT_XFERINFOFUNCTION,cancel);curl_easy_setopt(ws,CURLOPT_NOSIGNAL,1L);
    CURLcode rc=curl_easy_perform(ws);
    if(rc!=CURLE_OK || !send_bytes((unsigned char*)request,strlen(request))){snprintf(error,cap,"Signaling connection failed: %s",curl_easy_strerror(rc));return false;}
    u64 start=armGetSystemTick();
    while(!call_cancelled() && armTicksToNs(armGetSystemTick()-start)<10000000000ULL){
        receive[receive_len]=0;
        char *end=strstr((char*)receive,"\r\n\r\n");
        if(end){
            size_t size=(unsigned char*)end-receive+4;
            bool valid=!strncmp((char*)receive,"HTTP/1.1 101",12);
            char *p=(char*)receive;bool matched=false;
            while(p<end){char *next=strstr(p,"\r\n");if(!next)break;if(!strncasecmp(p,"Sec-WebSocket-Accept:",21)){p+=21;while(*p==' ')++p;matched=(size_t)(next-p)==strlen((char*)accept) && !memcmp(p,accept,next-p);}p=next+2;}
            if(!valid || !matched){snprintf(error,cap,"Invalid signaling WebSocket upgrade response");return false;}
            memmove(receive,receive+size,receive_len-size);receive_len-=size;return true;
        }
        if(fill()<0)break;svcSleepThread(5000000);
    }
    snprintf(error,cap,"Signaling WebSocket upgrade timed out");return false;
}
bool call_ws_send(cJSON *obj){char *s=cJSON_PrintUnformatted(obj);bool ok=s && frame(1,s,strlen(s));cJSON_free(s);return ok;}
cJSON *call_ws_next(bool *closed){
    *closed=false;if(failed || !ws){*closed=true;return NULL;}
    for(int i=0;i<8;i++){
        if(receive_len<2){if(fill()<0){*closed=true;return NULL;}if(receive_len<2)return NULL;}
        unsigned first=receive[0],second=receive[1];size_t n=second&127,head=2;
        if(second&128 || first&0x70){*closed=true;return NULL;}
        if(n==126){if(receive_len<4){if(fill()<0)*closed=true;return NULL;}n=((size_t)receive[2]<<8)|receive[3];head=4;}
        if(n==127 || n+head>=sizeof(receive)){*closed=true;return NULL;}
        if(receive_len<head+n){if(fill()<0)*closed=true;return NULL;}
        unsigned opcode=first&15;
        if((opcode>=8 && (!(first&128) || n>125)) || (opcode!=0 && opcode!=1 && opcode!=8 && opcode!=9 && opcode!=10)){*closed=true;return NULL;}
        if(opcode==8){*closed=true;return NULL;}
        if(opcode==9)frame(10,receive+head,n);
        else if(opcode==1 || opcode==0){
            if(opcode==1)message_len=0;
            if(message_len+n>=sizeof(message)){*closed=true;return NULL;}
            memcpy(message+message_len,receive+head,n);message_len+=n;
            if(first&128){message[message_len]=0;cJSON *obj=cJSON_Parse((char*)message);message_len=0;memmove(receive,receive+head+n,receive_len-head-n);receive_len-=head+n;return obj;}
        }
        memmove(receive,receive+head+n,receive_len-head-n);receive_len-=head+n;
    }
    return NULL;
}
void call_ws_close(void){if(ws)curl_easy_cleanup(ws);ws=NULL;receive_len=message_len=0;memset(receive,0,sizeof(receive));memset(message,0,sizeof(message));}
