/* Windows adapter for the actual notification filesystem tests. */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
DIR *opendir(const char *path){char pattern[1024];snprintf(pattern,sizeof(pattern),"%s/*",path);DIR *d=calloc(1,sizeof(*d));if(!d)return 0;d->handle=_findfirst(pattern,&d->data);if(d->handle==-1){free(d);return 0;}d->first=1;return d;}
struct dirent *readdir(DIR *d){if(d->first)d->first=0;else if(_findnext(d->handle,&d->data))return 0;d->entry.d_name=d->data.name;return &d->entry;}
int closedir(DIR *d){int rc=_findclose(d->handle);free(d);return rc;}
