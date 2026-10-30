#include "network.h"
#include "avatars.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static volatile LONG image_pending,image_started,image_cancelled;
void avatars_init(void){}void avatars_clear(void){InterlockedExchange(&image_pending,0);}void avatars_shutdown(void){}
bool avatars_pending(void){return image_pending!=0;}
void talk_set_poll_wait(unsigned seconds){assert(seconds==1 || seconds==8);}
void avatars_step(const TalkClient *t,AvatarCancel cancel,void *ctx){
 (void)t;InterlockedIncrement(&image_started);
 for(int i=0;i<2000;i++){if(cancel(ctx)){InterlockedIncrement(&image_cancelled);return;}Sleep(1);}
 InterlockedExchange(&image_pending,0);
}
static bool (*cancel_fn)(void*);static void *cancel_arg;
static volatile LONG polls,sends,arriving,cancelled_polls,fail_next,hold_send,held_send_started;
void talk_set_cancel(bool (*f)(void*),void *p){cancel_fn=f;cancel_arg=p;}
void talk_init(TalkClient *t){memset(t,0,sizeof(*t));}
void talk_clear_password(TalkClient *t){memset(t->app_password,0,sizeof(t->app_password));}
bool talk_rooms(TalkClient *t){t->room_count=1;strcpy(t->rooms[0].token,"room1");return true;}
bool talk_login(TalkClient *t){return talk_rooms(t);}
bool talk_messages(TalkClient *t,const char *token){
    t->message_count=0;t->poll_cursor=1;TalkMessage m={.id=1};talk_merge_message(t,&m);return true;
}
bool talk_poll(TalkClient *t,const char *token){
    InterlockedIncrement(&polls);
    if(InterlockedExchange(&fail_next,0)){strcpy(t->error,"offline");return false;}
    for(int i=0;i<3000;i++){
        if(cancel_fn(cancel_arg)){InterlockedIncrement(&cancelled_polls);return false;}
        LONG id=InterlockedExchange(&arriving,0);
        if(id){TalkMessage m={.id=id};talk_merge_message(t,&m);t->poll_cursor=id;return true;}
        Sleep(1);
    }
    return true;
}
bool talk_send(TalkClient *t,const char *token,const char *message){
    assert(!strcmp(message,"hello"));InterlockedIncrement(&sends);
    if(hold_send){InterlockedExchange(&held_send_started,1);while(hold_send){assert(!network_idle_for_update());Sleep(1);}}
    TalkMessage m={.id=3};talk_merge_message(t,&m);return true;
}
static TalkClient ui;
static bool await_action(NetAction expected){
    for(int i=0;i<5000;i++){
        NetAction a;bool ok;char n[240];
        if(network_take(&ui,&a,&ok,n,sizeof(n)) && a==expected)return ok;
        Sleep(1);
    }
    return false;
}
int main(void){
    assert(!network_start());
    strcpy(ui.server,"https://test.example");strcpy(ui.username,"user");strcpy(ui.app_password,"password");
    assert(network_submit(NET_LOGIN,&ui,NULL,NULL));assert(await_action(NET_LOGIN));
    network_watch(NULL);assert(network_submit(NET_HISTORY,&ui,"room1",NULL));assert(await_action(NET_HISTORY));
    network_watch("room1");Sleep(200);InterlockedExchange(&arriving,2);
    assert(await_action(NET_POLL));assert(ui.message_count==2);
    Sleep(250);assert(network_submit(NET_SEND,&ui,"room1","hello"));
    assert(await_action(NET_SEND));assert(sends==1 && cancelled_polls>0);
    InterlockedExchange(&arriving,3);assert(await_action(NET_POLL));assert(ui.message_count==3);
    InterlockedExchange(&image_pending,1);
    for(int i=0;i<5000 && !image_started;i++)Sleep(1);assert(image_started);
    network_pause(true);assert(image_cancelled);LONG saved_images=image_started;Sleep(100);assert(image_started==saved_images);
    InterlockedExchange(&image_pending,0);LONG parked_polls=polls;Sleep(100);assert(polls==parked_polls);
    InterlockedExchange(&fail_next,1);network_pause(false);
    bool failed=false;
    for(int i=0;i<5000 && !failed;i++){
        NetAction a;bool ok;char n[240];
        if(network_take(&ui,&a,&ok,n,sizeof(n)) && a==NET_POLL && !ok){
            assert(strstr(n,"retrying") && ui.message_count==3);failed=true;
        }
        Sleep(1);
    }
    assert(failed && polls>parked_polls);
    assert(network_submit(NET_HISTORY,&ui,"room1",NULL));assert(await_action(NET_HISTORY));
    network_signout();Sleep(250);
    NetAction a;bool ok;char n[240];assert(!network_take(&ui,&a,&ok,n,sizeof(n)));
    assert(network_submit(NET_LOGIN,&ui,NULL,NULL));assert(await_action(NET_LOGIN));
    InterlockedExchange(&hold_send,1);assert(network_submit(NET_SEND,&ui,"room1","hello"));assert(!network_idle_for_update());
    for(int i=0;i<5000 && !held_send_started;i++)Sleep(1);assert(held_send_started);
    network_watch(NULL);assert(!network_idle_for_update());InterlockedExchange(&hold_send,0);
    for(int i=0;i<5000 && !network_idle_for_update();i++)Sleep(1);assert(network_idle_for_update());
    network_stop();assert(!strcmp(ui.username,"user"));
    puts("Worker login, live receive, interrupted polling for send, dedup, call pause/resume and sign-out: PASS");return 0;
}
