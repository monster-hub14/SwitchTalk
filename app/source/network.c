/* One worker owns every HTTP/JSON/TLS operation; UI only exchanges snapshots. */
#include "network.h"
#include "avatars.h"
#include <stdatomic.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

typedef struct {
    NetAction action;
    char server[512],username[128],password[256],token[128],message[1200];
} Job;
static Thread worker;
static Mutex guard;
static atomic_bool stopping,interrupt_poll,paused,parked;
static Job queued;
static TalkClient remote,published;
static bool pending,busy,ready,result_ok,signed_in,wipe;
static NetAction result_action;
static char watched[128],result_notice[240];
static unsigned generation;
static bool polling;
static void erase(void *mem,size_t size){volatile unsigned char *p=mem;while(size--)*p++=0;}
static bool cancelled(void *ctx){
    (void)ctx;
    return atomic_load(&stopping) || (polling && atomic_load(&interrupt_poll));
}
static void run(void *ctx){
    (void)ctx;
    talk_set_cancel(cancelled,NULL);
    time_t retry_at=0,rooms_at=0;
    unsigned failures=0;bool avatar_turn=true;
    while(!atomic_load(&stopping)){
        if(atomic_load(&paused)){
            atomic_store(&parked,true);svcSleepThread(10000000);continue;
        }
        atomic_store(&parked,false);
        Job job={0};unsigned gen;bool have=false,image_job=false;
        mutexLock(&guard);
        if(wipe){erase(&remote,sizeof(remote));avatars_clear();signed_in=false;wipe=false;failures=0;retry_at=0;}
        gen=generation;
        if(!ready && pending){job=queued;erase(&queued,sizeof(queued));pending=false;have=true;polling=false;}
        else if(!ready && signed_in && time(NULL)>=retry_at){
            if(watched[0] && avatar_turn && avatars_pending()){image_job=have=true;avatar_turn=false;}
            else if(watched[0]){job.action=NET_POLL;snprintf(job.token,sizeof(job.token),"%s",watched);have=true;}
            else if(time(NULL)>=rooms_at){job.action=NET_AUTO_ROOMS;have=true;}
            else if(avatars_pending()){image_job=have=true;}
            polling=have;
        }
        if(have)atomic_store(&interrupt_poll,false);
        mutexUnlock(&guard);
        if(!have){svcSleepThread(50000000);continue;}
        if(image_job){
            avatars_step(&remote,cancelled,NULL);continue;
        }
        bool ok=false;
        switch(job.action){
            case NET_LOGIN:
                avatars_clear();avatar_turn=true;
                talk_init(&remote);
                snprintf(remote.server,sizeof(remote.server),"%s",job.server);
                snprintf(remote.username,sizeof(remote.username),"%s",job.username);
                snprintf(remote.app_password,sizeof(remote.app_password),"%s",job.password);
                ok=talk_login(&remote);break;
            case NET_ROOMS:case NET_AUTO_ROOMS:ok=talk_rooms(&remote);break;
            case NET_HISTORY:ok=talk_messages(&remote,job.token);break;
            case NET_SEND:ok=talk_send(&remote,job.token,job.message);break;
            case NET_POLL:talk_set_poll_wait(avatars_pending()?1:8);ok=talk_poll(&remote,job.token);avatar_turn=true;break;
        }
        mutexLock(&guard);
        bool stale=gen!=generation || (polling && atomic_load(&interrupt_poll));
        if(!stale){
            if(job.action==NET_LOGIN)signed_in=ok;
            if(job.action==NET_AUTO_ROOMS || job.action==NET_ROOMS || job.action==NET_LOGIN)rooms_at=time(NULL)+10;
            bool automatic=job.action==NET_POLL || job.action==NET_AUTO_ROOMS;
            if(automatic && !ok){
                if(failures<4)++failures;
                unsigned delay=1u<<failures;
                retry_at=time(NULL)+delay;
                snprintf(result_notice,sizeof(result_notice),"Connection interrupted; retrying in %us",delay);
            }else{
                failures=0;retry_at=0;
                snprintf(result_notice,sizeof(result_notice),"%s",
                    job.action==NET_SEND?"Message sent":job.action==NET_POLL?"Live messages connected":"Updated from server");
            }
            published=remote;result_ok=ok;result_action=job.action;ready=true;
            if(!automatic)busy=false;
        }
        erase(&job,sizeof(job));
        mutexUnlock(&guard);
        /* Avoid a request storm if a proxy/server returns an immediate empty response. */
        if(polling)svcSleepThread(150000000);
    }
    erase(&remote,sizeof(remote));
}
Result network_start(void){
    avatars_init();
    mutexInit(&guard);atomic_init(&paused,false);atomic_init(&parked,false);atomic_init(&stopping,false);atomic_init(&interrupt_poll,false);
    Result rc=threadCreate(&worker,run,NULL,NULL,256*1024,0x2c,-2);
    if(R_FAILED(rc))return rc;
    rc=threadStart(&worker);
    if(R_FAILED(rc))threadClose(&worker);
    return rc;
}
void network_stop(void){
    atomic_store(&stopping,true);atomic_store(&interrupt_poll,true);
    threadWaitForExit(&worker);threadClose(&worker);
    erase(&queued,sizeof(queued));erase(&published,sizeof(published));avatars_shutdown();
}
bool network_submit(NetAction action,const TalkClient *client,const char *token,const char *message){
    mutexLock(&guard);
    if(busy || pending){mutexUnlock(&guard);return false;}
    erase(&queued,sizeof(queued));queued.action=action;
    if(action==NET_LOGIN){
        snprintf(queued.server,sizeof(queued.server),"%s",client->server);
        snprintf(queued.username,sizeof(queued.username),"%s",client->username);
        snprintf(queued.password,sizeof(queued.password),"%s",client->app_password);
    }
    if(token)snprintf(queued.token,sizeof(queued.token),"%s",token);
    if(message)snprintf(queued.message,sizeof(queued.message),"%s",message);
    pending=busy=true;atomic_store(&interrupt_poll,true);
    mutexUnlock(&guard);return true;
}
bool network_take(TalkClient *client,NetAction *action,bool *ok,char *notice,size_t cap){
    mutexLock(&guard);
    if(!ready){mutexUnlock(&guard);return false;}
    *client=published;*action=result_action;*ok=result_ok;
    snprintf(notice,cap,"%s",result_notice);ready=false;
    erase(&published,sizeof(published));
    mutexUnlock(&guard);return true;
}
void network_watch(const char *token){
    mutexLock(&guard);
    snprintf(watched,sizeof(watched),"%s",token?token:"");
    ++generation;ready=pending=busy=false;erase(&queued,sizeof(queued));atomic_store(&interrupt_poll,true);
    mutexUnlock(&guard);
}
void network_signout(void){
    mutexLock(&guard);
    ++generation;watched[0]=0;ready=pending=busy=false;wipe=true;
    erase(&queued,sizeof(queued));erase(&published,sizeof(published));
    atomic_store(&interrupt_poll,true);mutexUnlock(&guard);
}

void network_pause(bool pause){
    atomic_store(&paused,pause);
    if(pause){atomic_store(&interrupt_poll,true);while(!atomic_load(&parked))svcSleepThread(10000000);}
}
