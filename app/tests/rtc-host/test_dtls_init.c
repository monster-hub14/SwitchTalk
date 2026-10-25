#include <windows.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define ADDRESS_H_
typedef struct Address { int unused; } Address;
#include "dtls_srtp.h"
#include "socket.h"
static int fail_entropy;
int mbedtls_hardware_poll(void *ctx,unsigned char *out,size_t len,size_t *olen){
    static uint32_t state=0xa37c591b;size_t i;(void)ctx;
    if(fail_entropy)return -1;
    for(i=0;i<len;i++){state^=state<<13;state^=state>>17;state^=state<<5;out[i]=(unsigned char)state;}
    *olen=len;return 0;
}
int udp_socket_sendto(UdpSocket *s,Address *a,const uint8_t *b,int n){return -1;}
int udp_socket_recvfrom(UdpSocket *s,Address *a,uint8_t *b,int n){return -1;}
void ports_sleep_ms(int ms){}
srtp_err_status_t srtp_create(srtp_t *s,const srtp_policy_t *p){return srtp_err_status_fail;}
srtp_err_status_t srtp_dealloc(srtp_t s){return srtp_err_status_ok;}
void srtp_crypto_policy_set_rtp_default(srtp_crypto_policy_t *p){}
void srtp_crypto_policy_set_rtcp_default(srtp_crypto_policy_t *p){}
srtp_err_status_t srtp_protect(srtp_t s,void *p,int *n){return srtp_err_status_fail;}
srtp_err_status_t srtp_unprotect(srtp_t s,void *p,int *n){return srtp_err_status_fail;}
srtp_err_status_t srtp_protect_rtcp(srtp_t s,void *p,int *n){return srtp_err_status_fail;}
srtp_err_status_t srtp_unprotect_rtcp(srtp_t s,void *p,int *n){return srtp_err_status_fail;}
int main(void){
    int i;
    for(i=0;i<4;i++){
        DtlsSrtp ctx;memset(&ctx,0,sizeof(ctx));
        assert(dtls_srtp_init(&ctx,i%2?DTLS_SRTP_ROLE_CLIENT:DTLS_SRTP_ROLE_SERVER,NULL)==0);
        assert(ctx.ctr_drbg.aes_ctx.rk && ctx.cert.raw.len>0 && strlen(ctx.local_fingerprint)==95);
        unsigned char random[32];assert(mbedtls_ctr_drbg_random(&ctx.ctr_drbg,random,sizeof(random))==0);
        dtls_srtp_deinit(&ctx);assert(!ctx.initialized);dtls_srtp_deinit(&ctx);
    }
    fail_entropy=1;
    for(i=0;i<2;i++){
        DtlsSrtp ctx;memset(&ctx,0,sizeof(ctx));
        assert(dtls_srtp_init(&ctx,i?DTLS_SRTP_ROLE_CLIENT:DTLS_SRTP_ROLE_SERVER,NULL)!=0);
        dtls_srtp_deinit(&ctx);assert(!ctx.initialized);
    }
    puts("Real Mbed TLS DTLS initialization: server cookie RNG, client/server certificates, repeat cleanup and entropy failure: PASS");return 0;
}

int gmtime_s(struct tm *out,const time_t *t){struct tm *p=gmtime(t);if(!p)return 1;*out=*p;return 0;}
int gettimeofday(struct timeval *tv,void *tz){tv->tv_sec=(long)time(NULL);tv->tv_usec=0;return 0;}
