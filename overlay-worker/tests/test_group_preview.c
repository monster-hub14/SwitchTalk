#include "alert_policy.h"
#include "alert_preview.h"
#include "notify_file.h"
#include <assert.h>
#include <string.h>
static int outputs;
static bool emit(const AlertRoom *r,void *ctx){(void)ctx;char text[512];alert_preview(r,text,sizeof(text));assert(!strcmp(text,"Friends / Alice\nNew group message"));bool ok=notify_write(r->token,r->id,text);if(ok)++outputs;return ok;}
int main(void){
 notify_clear();notify_begin(0xabcdef);AlertPolicy p={0};AlertRoom r={.token="room",.title="Friends",.sender="Alice",.actor="alice",.body="New group message",.type=2,.level=2,.id=1};
 alert_scan(&p,&r,1,"me","me",emit,0);assert(!outputs);
 r.id=2;alert_scan(&p,&r,1,"me","me",emit,0);assert(outputs==1 && !p.stats.settings);
 r.id=3;alert_scan(&p,&r,1,"me","me",emit,0);assert(outputs==2);
 r.id=4;strcpy(r.actor,"me");alert_scan(&p,&r,1,"me","me",emit,0);assert(outputs==2 && p.stats.own==1);
 strcpy(r.actor,"alice");r.id=5;r.silent=true;alert_scan(&p,&r,1,"me","me",emit,0);assert(outputs==2 && p.stats.system==1);
 return 0;
}
