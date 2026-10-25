#pragma once
#include <stdbool.h>
#include <stddef.h>
/* Accept a bare host or an HTTPS base URL, preserving optional subdirectories. */
bool server_normalize(char *server,size_t capacity);
bool server_has_host(const char *server);
