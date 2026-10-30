/* Native libnx TLS trust setup and read-only handshake diagnostics. */
#include <switch.h>
#include <curl/curl.h>
#include <stdio.h>
#include "tls.h"
#include "tls_roots.h"

static _Thread_local Result last_handshake, last_verify, root_x1, root_x2;
static _Thread_local bool custom_ca;
static CURLcode import_roots(CURL *curl, void *sslctx, void *userdata) {
    (void)curl; (void)userdata;
    SslContext *ctx=(SslContext*)sslctx;
    root_x1=sslContextImportServerPki(ctx,isrg_root_x1,sizeof(isrg_root_x1),SslCertificateFormat_Der,NULL);
    root_x2=sslContextImportServerPki(ctx,isrg_root_x2,sizeof(isrg_root_x2),SslCertificateFormat_Der,NULL);
    return R_SUCCEEDED(root_x1)||R_SUCCEEDED(root_x2) ? CURLE_OK:CURLE_SSL_CACERT_BADFILE;
}
CURLcode talk_tls_configure(CURL *curl) {
    last_handshake=last_verify=root_x1=root_x2=0;
    FILE *ca=fopen("sdmc:/switch/SwitchTalk/ca.pem","rb");
    custom_ca=(ca!=NULL);
    if(ca){
        fclose(ca);
        return curl_easy_setopt(curl,CURLOPT_CAINFO,"sdmc:/switch/SwitchTalk/ca.pem");
    }
    return curl_easy_setopt(curl,CURLOPT_SSL_CTX_FUNCTION,import_roots);
}
/* GNU ld redirects libcurl's call to this function, preserving its result. */
Result __real_sslConnectionDoHandshake(SslConnection *c,u32 *size,u32 *count,void *buf,u32 cap);
Result __wrap_sslConnectionDoHandshake(SslConnection *c,u32 *size,u32 *count,void *buf,u32 cap) {
    Result rc=__real_sslConnectionDoHandshake(c,size,count,buf,cap);
    last_handshake=rc;
    if(R_FAILED(rc) && R_VALUE(rc)==MAKERESULT(123,207))
        last_verify=sslConnectionGetVerifyCertError(c);
    return rc;
}
void talk_tls_diagnostic(char *out,size_t cap) {
    snprintf(out,cap,"TLS hs=0x%08x verify=0x%08x; %s X1=0x%08x X2=0x%08x",
             last_handshake,last_verify,custom_ca?"custom CA":"ISRG roots",root_x1,root_x2);
}
