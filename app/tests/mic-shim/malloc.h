#ifndef MIC_TEST_MALLOC_H
#define MIC_TEST_MALLOC_H
#include <stdlib.h>
/* Host mocks do not use DMA. Device builds use libnx/newlib memalign. */
static void *memalign(size_t alignment,size_t n){return malloc(n);}
#endif
