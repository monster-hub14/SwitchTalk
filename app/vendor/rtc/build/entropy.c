#include <switch.h>
#include <stddef.h>
int mbedtls_hardware_poll(void *ctx,unsigned char *out,size_t len,size_t *olen){(void)ctx;randomGet(out,len);*olen=len;return 0;}
