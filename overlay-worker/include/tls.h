#ifndef SWITCHTALK_TLS_H
#define SWITCHTALK_TLS_H
#include <curl/curl.h>
#include <stddef.h>
CURLcode talk_tls_configure(CURL *curl);
void talk_tls_diagnostic(char *out, size_t cap);
#endif
