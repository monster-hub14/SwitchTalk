#pragma once
#include <io.h>
#include <stdint.h>
struct dirent {char *d_name;};
typedef struct {intptr_t handle;struct _finddata_t data;struct dirent entry;int first;} DIR;
DIR *opendir(const char*);struct dirent *readdir(DIR*);int closedir(DIR*);
