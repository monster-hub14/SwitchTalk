#include "talk.h"
#include <string.h>
void talk_merge_message(TalkClient *t,const TalkMessage *m) {
    if(m->id<=0)return;
    for(int i=0;i<t->message_count;i++)if(t->messages[i].id==m->id){t->messages[i]=*m;return;}
    if(t->message_count==TALK_MESSAGES_MAX){
        if(m->id<t->messages[0].id)return;
        memmove(t->messages,t->messages+1,(TALK_MESSAGES_MAX-1)*sizeof(*m));
        --t->message_count;
    }
    int i=t->message_count++;
    while(i>0 && t->messages[i-1].id>m->id){t->messages[i]=t->messages[i-1];--i;}
    t->messages[i]=*m;
}
