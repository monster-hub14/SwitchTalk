#ifndef TEST_WINSOCK2_H
#define TEST_WINSOCK2_H
#include <stdint.h>
#define _WINSOCKAPI_
typedef uintptr_t SOCKET;
#define INVALID_SOCKET ((SOCKET)-1)
struct sockaddr {unsigned short sa_family;char sa_data[14];};
typedef struct fd_set {unsigned int count;SOCKET values[64];} fd_set;

#endif
