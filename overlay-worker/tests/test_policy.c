#include "alert_policy.h"
#include <assert.h>
#include <string.h>
static unsigned emitted;static bool accept=true;
static bool output(const AlertRoom *r,void *p){(void)p;assert(r->id>0);++emitted;return accept;}
int main(void){
 AlertPolicy p={0};AlertRoom r={.token="room",.title="Room",.actor="other",.id=10,.unread=0,.level=0,.type=2};
 alert_scan(&p,&r,1,"me","me",output,0);assert(!emitted);
 long id=10;
 for(int type=1;type<=3;type++)for(int level=0;level<=3;level++){
  r.type=type;r.level=level;r.id=++id;unsigned before=emitted;alert_scan(&p,&r,1,"me","me",output,0);assert(emitted==before+1 && !p.stats.settings);
  alert_scan(&p,&r,1,"me","me",output,0);assert(emitted==before+1);
 }
 unsigned before=emitted;strcpy(r.actor,"me");r.id=++id;alert_scan(&p,&r,1,"login-alias","me",output,0);assert(emitted==before && p.stats.own==1);
 strcpy(r.actor,"other");r.system=true;r.id=++id;alert_scan(&p,&r,1,"me","me",output,0);assert(emitted==before && p.stats.system==1);
 r.system=false;r.silent=true;r.id=++id;alert_scan(&p,&r,1,"me","me",output,0);assert(emitted==before && p.stats.system==1);
 r.silent=false;r.id=++id;accept=false;long old=p.rooms[0].id;alert_scan(&p,&r,1,"me","me",output,0);assert(p.rooms[0].id==old && p.stats.blocked==1);
 accept=true;alert_scan(&p,&r,1,"me","me",output,0);assert(p.rooms[0].id==r.id);
 alert_scan(&p,0,0,"me","me",output,0);before=emitted;r.id=++id;alert_scan(&p,&r,1,"me","me",output,0);assert(emitted==before);
 return 0;
}
