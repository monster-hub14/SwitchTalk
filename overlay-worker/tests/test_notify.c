#include "notify_file.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 notify_clear();notify_begin(0x12345678);
 assert(notify_write("room",1,"Quote \" slash \\ newline\n tab\t \001"));
 assert(notify_write("room",1,"duplicate must not add another file"));
 for(int i=2;i<=8;i++)assert(notify_write("room",i,"new message"));
 assert(!notify_write("room",9,"bounded queue"));
 unsigned pending,real,test,errors;notify_diagnostics(&pending,&real,&test,&errors);assert(pending==8 && real==1 && test==0 && errors==0);
 char path[512],copy[1024];snprintf(path,sizeof(path),NOTIFY_DIR "/switchtalk-000000001234567800000008.notify");FILE *f=fopen(path,"rb");assert(f);size_t n=fread(copy,1,sizeof(copy),f);fclose(f);assert(!remove(path));
 notify_diagnostics(&pending,&real,&test,&errors);assert(pending==7 && real==2 && errors==0);
 f=fopen(path,"wb");assert(f && fwrite(copy,1,n,f)==n);assert(!fclose(f));
 return 0;
}
