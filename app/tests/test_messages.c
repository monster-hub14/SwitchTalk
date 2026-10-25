#include "talk.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static TalkClient t;
int main(void){
    TalkMessage m={.id=3};strcpy(m.body,"three");talk_merge_message(&t,&m);
    m.id=1;talk_merge_message(&t,&m);m.id=2;talk_merge_message(&t,&m);
    assert(t.message_count==3 && t.messages[0].id==1 && t.messages[2].id==3);
    strcpy(m.body,"edited");talk_merge_message(&t,&m);
    assert(t.message_count==3 && !strcmp(t.messages[1].body,"edited"));
    for(int i=4;i<=140;i++){m.id=i;talk_merge_message(&t,&m);}
    assert(t.message_count==TALK_MESSAGES_MAX && t.messages[0].id==41 && t.messages[99].id==140);
    m.id=5;talk_merge_message(&t,&m);assert(t.messages[0].id==41 && t.message_count==100);
    m.id=0;talk_merge_message(&t,&m);assert(t.message_count==100);
    puts("Message ordering, deduplication, updates and rolling history: PASS");return 0;
}
