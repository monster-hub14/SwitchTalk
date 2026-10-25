#pragma once
#include <stdint.h>
#include <stddef.h>
typedef uint32_t Result;
#define R_FAILED(x) ((x)!=0)
Result splCryptoInitialize(void);void splCryptoExit(void);
Result splCryptoGenerateAesKek(const void*,uint32_t,uint32_t,void*);
Result splCryptoGenerateAesKey(const void*,const void*,void*);
Result csrngInitialize(void);Result csrngGetRandomBytes(void*,size_t);void csrngExit(void);
