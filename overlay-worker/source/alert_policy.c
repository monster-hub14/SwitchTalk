#include "alert_policy.h"
#include <string.h>
#include <stdio.h>
void alert_scan(AlertPolicy *p,const AlertRoom *rooms,int count,const char *user,const char *account,AlertEmit emit,void *ctx){
 bool seen[ALERT_ROOMS]={0};memset(&p->stats,0,sizeof(p->stats));
 for(int n=0;n<count && n<ALERT_ROOMS;n++){
  const AlertRoom *r=&rooms[n];if(!r->token[0])continue;++p->stats.rooms;if(r->id>0)++p->stats.ids;if(r->id>p->stats.max_id)p->stats.max_id=r->id;int slot=-1;
  for(int i=0;i<ALERT_ROOMS;i++)if(p->rooms[i].active && !strcmp(p->rooms[i].token,r->token)){slot=i;break;}
  if(slot<0){
   for(int i=0;i<ALERT_ROOMS;i++)if(!p->rooms[i].active){slot=i;break;}
   if(slot<0)continue;
   p->rooms[slot].active=true;snprintf(p->rooms[slot].token,sizeof(p->rooms[slot].token),"%s",r->token);p->rooms[slot].id=r->id;seen[slot]=true;++p->stats.baselines;continue;
  }
  seen[slot]=true;AlertCursor *c=&p->rooms[slot];if(r->id<=c->id)continue;
  ++p->stats.changes;
  bool own=r->actor[0] && ((!strcmp(r->actor,user)) || (account[0] && !strcmp(r->actor,account)));
  /* Every conversation uses the same fixed incoming-message policy. */
  const char *reason="queued";bool commit=true;
  if(!p->initialized){++p->stats.baselines;reason="baseline";}
  else if(own){++p->stats.own;reason="own message";}
  else if(r->system || r->silent){++p->stats.system;reason="system/silent event";}
  else if(!emit(r,ctx)){++p->stats.blocked;reason="queue write blocked";commit=false;}
  if(commit)c->id=r->id;
  snprintf(p->stats.last,sizeof(p->stats.last),"%.40s ID %ld: %s",r->title[0]?r->title:r->token,r->id,reason);
 }
 for(int i=0;i<ALERT_ROOMS;i++)if(!seen[i])p->rooms[i].active=false;
 p->initialized=true;
}
