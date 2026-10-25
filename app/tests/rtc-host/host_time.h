#include <windows.h>
#include <time.h>
int gmtime_s(struct tm *out,const time_t *t);
int gettimeofday(struct timeval *tv,void *tz);
