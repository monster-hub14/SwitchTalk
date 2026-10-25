/* HPB/WebRTC audio conference engine. UI runs separately on the main thread. */
#include <switch.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "call.h"
#include "call_protocol.h"
#include <time.h>
#include "call_http.h"
#include "mic_input.h"
#include "call_engine.h"
#include "peer_connection.h"
#include <opus/opus.h>
#define REMOTES 5
#define FRAME 960
#define RING (FRAME*2*24)
extern int peer_init(void);
extern void peer_deinit(void);
typedef struct {PeerConnection *pc;char session[1024],sid[128],mid[64];OpusDecoder *decoder;int16_t ring[RING];unsigned read,count;bool requested;time_t retry;} Remote;
static Remote remotes[REMOTES],publisher;
static PeerConfiguration rtc_config;
static OpusEncoder *encoder;
static TalkClient account;
static char room[128],room_title[192],own_session[1024],nc_session[1024],status[240],microphone[256];
static cJSON *settings,*pending_events;
static Mutex state_guard;
static atomic_bool stopping,finished,muted;
static atomic_uint sent,received,linked;
static bool cleanup;
static MicState mic;
static int16_t encode_samples[FRAME],decode_samples[5760*2];static unsigned encode_used;
static AudioOutBuffer output_buffers[3];
static int16_t speaker_data[3][2048] __attribute__((aligned(4096)));
static bool audio_open,peer_open,in_call,in_room;
bool call_cancelled(void){return atomic_load(&stopping) && !cleanup;}
void call_set_status(const char *s){mutexLock(&state_guard);snprintf(status,sizeof(status),"%s",s);mutexUnlock(&state_guard);}
static void publish_json(cJSON *o){if(!call_ws_send(o) && !call_cancelled()){call_set_status("Signaling send failed. End call and retry.");atomic_store(&stopping,true);}cJSON_Delete(o);}
static cJSON *packet(const char *type){cJSON *o=cJSON_CreateObject();cJSON_AddStringToObject(o,"type",type);return o;}
static cJSON *signal_message(Remote *p,const char *type,const char *sdp){
    cJSON *o=packet("message"),*m=cJSON_AddObjectToObject(o,"message"),*recipient=cJSON_AddObjectToObject(m,"recipient"),*data=cJSON_AddObjectToObject(m,"data");
    cJSON_AddStringToObject(recipient,"type","session");cJSON_AddStringToObject(recipient,"sessionid",p->session);
    cJSON_AddStringToObject(data,"to",p->session);cJSON_AddStringToObject(data,"type",type);cJSON_AddStringToObject(data,"roomType","video");cJSON_AddStringToObject(data,"sid",p->sid);
    cJSON_AddStringToObject(o,"id",p->sid);
    if(sdp){cJSON *payload=cJSON_AddObjectToObject(data,"payload");cJSON_AddStringToObject(payload,"type",type);cJSON_AddStringToObject(payload,"sdp",sdp);cJSON_AddStringToObject(payload,"nick",account.username);cJSON_AddStringToObject(data,"audiocodec","opus");}
    return o;
}
static cJSON *wait_type(const char *type,unsigned seconds,char *error,size_t cap){
    u64 start=armGetSystemTick();
    while(!call_cancelled() && armTicksToNs(armGetSystemTick()-start)<(u64)seconds*1000000000ULL){
        bool closed=false;cJSON *o=call_ws_next(&closed);
        if(closed){snprintf(error,cap,"Signaling connection closed during %s",type);return NULL;}
        if(o){
            const char *t=call_json_string(o,"type");
            if(!strcmp(t,type))return o;
            if(!strcmp(t,"error")){cJSON *e=cJSON_GetObjectItemCaseSensitive(o,"error");snprintf(error,cap,"HPB %.60s: %.140s",call_json_string(e,"code"),call_json_string(e,"message"));cJSON_Delete(o);return NULL;}
            if(!strcmp(t,"event"))cJSON_AddItemToArray(pending_events,o);
            else cJSON_Delete(o);
        }
        svcSleepThread(5000000);
    }
    snprintf(error,cap,"Timed out waiting for signaling %s",type);return NULL;
}
static void audio_received(uint8_t *packet_data,size_t bytes,void *ctx){
    Remote *p=ctx;if(!p->decoder || bytes>1500)return;
    int frames=opus_decode(p->decoder,packet_data,(opus_int32)bytes,decode_samples,5760,0);
    if(frames<0)return;
    unsigned samples=(unsigned)frames*2;
    for(unsigned i=0;i<samples;i++){
        if(p->count==RING){p->read=(p->read+1)%RING;--p->count;}
        p->ring[(p->read+p->count)%RING]=decode_samples[i];++p->count;
    }
    atomic_fetch_add(&received,1);
}
static void samples_received(const int16_t *samples,size_t frames,unsigned rate,unsigned channels,void *ctx){
    (void)ctx;if(rate!=48000 || !encoder)return;
    for(size_t i=0;i<frames;i++){
        int value=samples[i*channels];if(channels==2)value=(value+samples[i*channels+1])/2;
        encode_samples[encode_used++]=atomic_load(&muted)?0:(int16_t)value;
        if(encode_used==FRAME){
            unsigned char packet_data[1300];int n=opus_encode(encoder,encode_samples,FRAME,packet_data,sizeof(packet_data));encode_used=0;
            if(n>0 && publisher.pc && peer_connection_get_state(publisher.pc)==PEER_CONNECTION_CONNECTED){
                if(peer_connection_send_audio(publisher.pc,packet_data,(size_t)n)>=0)atomic_fetch_add(&sent,1);
            }
        }
    }
}
static bool open_audio(char *error,size_t cap){
    mic_rescan(&mic);
    if(!mic.capturing){snprintf(error,cap,"Microphone: %.200s",mic.status);return false;}
    if(mic.rate!=48000){snprintf(error,cap,"Call capture needs 48000 Hz; device returned %u",mic.rate);return false;}
    snprintf(microphone,sizeof(microphone),"%s",mic.active);
    mic_set_sink(samples_received,NULL);
    int code;encoder=opus_encoder_create(48000,1,OPUS_APPLICATION_VOIP,&code);
    if(!encoder){snprintf(error,cap,"Opus encoder failed: %d",code);return false;}
    opus_encoder_ctl(encoder,OPUS_SET_BITRATE(24000));opus_encoder_ctl(encoder,OPUS_SET_DTX(1));
    Result rc=audoutInitialize();if(R_FAILED(rc)){snprintf(error,cap,"Call output failed: 0x%08x",rc);return false;}
    audio_open=true;
    if(audoutGetSampleRate()!=48000 || audoutGetChannelCount()!=2 || audoutGetPcmFormat()!=PcmFormat_Int16){snprintf(error,cap,"Call output format is unsupported");return false;}
    memset(output_buffers,0,sizeof(output_buffers));memset(speaker_data,0,sizeof(speaker_data));
    for(unsigned i=0;i<3;i++){
        output_buffers[i].buffer=speaker_data[i];output_buffers[i].buffer_size=4096;output_buffers[i].data_size=FRAME*2*sizeof(int16_t);
        rc=audoutAppendAudioOutBuffer(&output_buffers[i]);if(R_FAILED(rc))break;
    }
    if(R_SUCCEEDED(rc))rc=audoutStartAudioOut();
    if(R_FAILED(rc)){snprintf(error,cap,"Call playback failed: 0x%08x",rc);return false;}
    return true;
}
static void play_audio(void){
    for(int n=0;n<3;n++){
        AudioOutBuffer *b=NULL;u32 count=0;Result rc=audoutGetReleasedAudioOutBuffer(&b,&count);
        if(R_FAILED(rc) || !count || !b)return;
        int16_t *out=b->buffer;
        for(unsigned i=0;i<FRAME*2;i++){
            int sum=0;
            for(unsigned p=0;p<REMOTES;p++)if(remotes[p].count){sum+=remotes[p].ring[remotes[p].read];remotes[p].read=(remotes[p].read+1)%RING;--remotes[p].count;}
            if(sum>32767)sum=32767;if(sum<-32768)sum=-32768;out[i]=(int16_t)sum;
        }
        audoutAppendAudioOutBuffer(b);
    }
}
static PeerConnection *make_peer(Remote *p,bool receiving){
    PeerConfiguration config=rtc_config;
    config.audio_codec=CODEC_OPUS;config.video_codec=CODEC_NONE;config.datachannel=DATA_CHANNEL_NONE;
    config.user_data=p;config.onaudiotrack=receiving?audio_received:NULL;
    if(receiving){int e;p->decoder=opus_decoder_create(48000,2,&e);if(!p->decoder)return NULL;}
    return peer_connection_create(&config);
}
static void close_peer(Remote *p){
    if(p->pc){peer_connection_close(p->pc);peer_connection_destroy(p->pc);}if(p->decoder)opus_decoder_destroy(p->decoder);memset(p,0,sizeof(*p));
}
static void request_remote(const char *session){
    if(!session[0] || !strcmp(session,own_session))return;
    for(unsigned i=0;i<REMOTES;i++)if(!strcmp(remotes[i].session,session))return;
    for(unsigned i=0;i<REMOTES;i++)if(!remotes[i].session[0]){
        Remote *p=&remotes[i];snprintf(p->session,sizeof(p->session),"%s",session);snprintf(p->sid,sizeof(p->sid),"switchtalk-sub-%u",i);p->requested=true;p->retry=time(NULL)+15;
        publish_json(signal_message(p,"requestoffer",NULL));return;
    }
}
static void handle(cJSON *o){
    const char *type=call_json_string(o,"type");
    if(!strcmp(type,"error")){cJSON *e=cJSON_GetObjectItemCaseSensitive(o,"error");char s[240];snprintf(s,sizeof(s),"HPB %.60s: %.140s",call_json_string(e,"code"),call_json_string(e,"message"));call_set_status(s);
        const char *id=call_json_string(o,"id");
        for(unsigned i=0;i<REMOTES;i++)if(!remotes[i].pc && !strcmp(remotes[i].sid,id)){remotes[i].requested=false;remotes[i].retry=time(NULL)+2;}
        return;}
    if(!strcmp(type,"event")){
        cJSON *event=cJSON_GetObjectItemCaseSensitive(o,"event");const char *target=call_json_string(event,"target"),*kind=call_json_string(event,"type");
        if(!strcmp(target,"participants") && !strcmp(kind,"update")){
            cJSON *update=cJSON_GetObjectItemCaseSensitive(event,"update");
            if(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(update,"all"))){cJSON *inc=cJSON_GetObjectItemCaseSensitive(update,"incall");if(in_call && cJSON_IsNumber(inc) && !inc->valueint){call_set_status("Conference ended by the room");atomic_store(&stopping,true);}return;}
            cJSON *users=cJSON_GetObjectItemCaseSensitive(update,"users");
            cJSON *u; cJSON_ArrayForEach(u,users){cJSON *incall=cJSON_GetObjectItemCaseSensitive(u,"inCall");if(!incall)incall=cJSON_GetObjectItemCaseSensitive(u,"incall");const char *session=call_json_string(u,"sessionId");
                if(cJSON_IsNumber(incall) && (incall->valueint&1))request_remote(session);
                else if(cJSON_IsNumber(incall))for(unsigned i=0;i<REMOTES;i++)if(!strcmp(remotes[i].session,session))close_peer(&remotes[i]);}
        }
        if(!strcmp(target,"room") && !strcmp(kind,"leave")){
            cJSON *s;cJSON_ArrayForEach(s,cJSON_GetObjectItemCaseSensitive(event,"leave"))if(cJSON_IsString(s))for(unsigned i=0;i<REMOTES;i++)if(!strcmp(remotes[i].session,s->valuestring))close_peer(&remotes[i]);
        }
        return;
    }
    if(strcmp(type,"message"))return;
    cJSON *m=cJSON_GetObjectItemCaseSensitive(o,"message"),*sender=cJSON_GetObjectItemCaseSensitive(m,"sender"),*data=cJSON_GetObjectItemCaseSensitive(m,"data"),*payload=cJSON_GetObjectItemCaseSensitive(data,"payload");
    const char *session=call_json_string(sender,"sessionid"),*kind=call_json_string(data,"type");
    Remote *p=NULL;if(!strcmp(session,own_session))p=&publisher;
    else for(unsigned i=0;i<REMOTES;i++)if(!strcmp(session,remotes[i].session))p=&remotes[i];
    if(!p)return;
    if(!strcmp(kind,"answer") && p==&publisher && p->pc){peer_connection_set_remote_description(p->pc,call_json_string(payload,"sdp"),SDP_TYPE_ANSWER);call_set_status("Publisher negotiated; connecting encrypted audio...");}
    else if(!strcmp(kind,"offer") && p!=&publisher){
        const char *sdp=call_json_string(payload,"sdp");if(!sdp[0] || strlen(sdp)>24000)return;
        if(p->pc)return;
        snprintf(p->sid,sizeof(p->sid),"%s",call_json_string(data,"sid"));p->pc=make_peer(p,true);if(!p->pc){call_set_status("Unable to allocate remote audio connection");return;}
        peer_connection_set_remote_description(p->pc,sdp,SDP_TYPE_OFFER);
        const char *answer=peer_connection_create_answer(p->pc);
        if(peer_connection_get_state(p->pc)==PEER_CONNECTION_FAILED){call_set_status("Unable to initialize encrypted remote audio transport");close_peer(p);return;}
        char *fixed=call_sdp_answer(answer,sdp,p->mid,sizeof(p->mid));
        if(!fixed){call_set_status("Unsupported SFU audio offer layout");close_peer(p);return;}
        publish_json(signal_message(p,"answer",fixed));free(fixed);
    }else if(!strcmp(kind,"candidate") && p->pc){
        cJSON *candidate=cJSON_GetObjectItemCaseSensitive(payload,"candidate");const char *value=call_json_string(candidate,"candidate");if(!value[0])value=call_json_string(candidate,"candiate");
        if(value[0] && strlen(value)<1024)peer_connection_add_ice_candidate(p->pc,(char*)value);
    }
}
static void parse_ice(void){
    memset(&rtc_config,0,sizeof(rtc_config));unsigned count=0;
    const char *fields[]={"stunservers"};
    for(unsigned f=0;f<1;f++){
        cJSON *server;cJSON_ArrayForEach(server,cJSON_GetObjectItemCaseSensitive(settings,fields[f])){
            cJSON *url;cJSON_ArrayForEach(url,cJSON_GetObjectItemCaseSensitive(server,"urls")){
                if(count>=5 || !cJSON_IsString(url))continue;
                if(strstr(url->valuestring,"transport=tcp") || !strncmp(url->valuestring,"turns:",6))continue;
                rtc_config.ice_servers[count].urls=url->valuestring;
                rtc_config.ice_servers[count].username=call_json_string(server,"username");rtc_config.ice_servers[count].credential=call_json_string(server,"credential");++count;
            }
        }
    }
}
static void run_call(void *ctx){
    (void)ctx;char path[2048],error[240]={0};cJSON *data=NULL,*answer=NULL;
    cleanup=false;pending_events=cJSON_CreateArray();call_http_init(&account);call_set_status("Fetching conference signaling settings...");
    snprintf(path,sizeof(path),"/ocs/v2.php/apps/spreed/api/v3/signaling/settings?token=%s&format=json",room);
    settings=call_http("GET",path,NULL,error,sizeof(error));if(!settings)goto end;
    cJSON *server=cJSON_GetObjectItemCaseSensitive(settings,"server");const char *url=cJSON_IsString(server)?server->valuestring:cJSON_IsArray(server)?cJSON_GetStringValue(cJSON_GetArrayItem(server,0)):NULL;
    if(!url || !url[0]){snprintf(error,sizeof(error),"No external HPB signaling server returned by Nextcloud");goto end;}
    call_http_use_session();
    parse_ice();call_set_status("Joining the Nextcloud room session...");
    snprintf(path,sizeof(path),"/ocs/v2.php/apps/spreed/api/v4/room/%s/participants/active?format=json",room);
    data=call_http("POST",path,"force=false",error,sizeof(error));if(!data)goto end;in_room=true;
    snprintf(nc_session,sizeof(nc_session),"%s",call_json_string(data,"sessionId"));cJSON_Delete(data);data=NULL;
    if(!nc_session[0]){snprintf(error,sizeof(error),"Nextcloud did not return a room session ID");goto end;}
    if(!call_http_check_room_session(room,nc_session,error,sizeof(error)))goto end;
    call_set_status("Opening secure HPB signaling connection...");
    if(!call_ws_open(url,account.server,error,sizeof(error)))goto end;
    cJSON *params=cJSON_GetObjectItemCaseSensitive(settings,"helloAuthParams"),*auth_params=cJSON_GetObjectItemCaseSensitive(params,"2.0");const char *version="2.0";
    if(!auth_params){auth_params=cJSON_GetObjectItemCaseSensitive(params,"1.0");version="1.0";}
    if(!auth_params){snprintf(error,sizeof(error),"HPB authentication parameters are missing");goto end;}
    cJSON *hello=packet("hello"),*h=cJSON_AddObjectToObject(hello,"hello"),*auth=cJSON_AddObjectToObject(h,"auth");
    cJSON_AddStringToObject(h,"version",version);snprintf(path,sizeof(path),"%s/ocs/v2.php/apps/spreed/api/v3/signaling/backend",account.server);
    cJSON_AddStringToObject(auth,"url",path);cJSON_AddItemToObject(auth,"params",cJSON_Duplicate(auth_params,true));publish_json(hello);
    answer=wait_type("hello",15,error,sizeof(error));if(!answer)goto end;
    snprintf(own_session,sizeof(own_session),"%s",call_json_string(cJSON_GetObjectItemCaseSensitive(answer,"hello"),"sessionid"));cJSON_Delete(answer);answer=NULL;
    if(!own_session[0]){snprintf(error,sizeof(error),"HPB did not issue a signaling session");goto end;}
    cJSON *join=packet("room"),*r=cJSON_AddObjectToObject(join,"room");cJSON_AddStringToObject(r,"roomid",room);cJSON_AddStringToObject(r,"sessionid",nc_session);publish_json(join);
    answer=wait_type("room",15,error,sizeof(error));if(!answer)goto end;cJSON_Delete(answer);answer=NULL;
    call_set_status("Opening microphone and conference audio...");
    if(!open_audio(error,sizeof(error)))goto end;
    if(peer_init()!=0){snprintf(error,sizeof(error),"SRTP initialization failed");goto end;}peer_open=true;
    publisher.pc=make_peer(&publisher,false);if(!publisher.pc){snprintf(error,sizeof(error),"Cannot allocate publisher transport");goto end;}
    snprintf(publisher.session,sizeof(publisher.session),"%s",own_session);strcpy(publisher.sid,"switchtalk-publisher");
    cJSON *pending_event; cJSON_ArrayForEach(pending_event,pending_events)handle(pending_event);
    cJSON_Delete(pending_events);pending_events=NULL;
    call_set_status("Starting/joining conference call...");snprintf(path,sizeof(path),"/ocs/v2.php/apps/spreed/api/v4/call/%s?format=json",room);
    data=call_http("POST",path,"flags=3",error,sizeof(error));if(!data)goto end;in_call=true;cJSON_Delete(data);data=NULL;
    call_set_status("Gathering encrypted audio connection candidates...");
    const char *offer=peer_connection_create_offer(publisher.pc);
    if(peer_connection_get_state(publisher.pc)==PEER_CONNECTION_FAILED){snprintf(error,sizeof(error),"Unable to initialize encrypted microphone transport");goto end;}
    publish_json(signal_message(&publisher,"offer",offer));
    call_set_status("Call started. Negotiating audio with the HPB...");
    bool old_mute=false,publisher_live=false,media_live=false;time_t connect_deadline=time(NULL)+45;
    while(!call_cancelled()){
        bool closed=false;
        for(int i=0;i<8;i++){cJSON *event=call_ws_next(&closed);if(!event)break;handle(event);cJSON_Delete(event);}
        if(closed){snprintf(error,sizeof(error),"HPB signaling disconnected; call ended");break;}
        if(publisher.pc){
            peer_connection_loop(publisher.pc);
            PeerConnectionState state=peer_connection_get_state(publisher.pc);
            if(state==PEER_CONNECTION_FAILED || state==PEER_CONNECTION_CLOSED){snprintf(error,sizeof(error),"Publisher ICE/DTLS transport failed (state %d)",state);break;}
            if(!publisher_live && state==PEER_CONNECTION_CONNECTED){publisher_live=true;call_set_status("Microphone transport connected. Waiting for remote audio...");}
        }
        for(unsigned i=0;i<REMOTES;i++)if(remotes[i].session[0] && !remotes[i].pc && time(NULL)>=remotes[i].retry){
            publish_json(signal_message(&remotes[i],"requestoffer",NULL));remotes[i].requested=true;remotes[i].retry=time(NULL)+15;
        }
        unsigned count=0;
        for(unsigned i=0;i<REMOTES;i++)if(remotes[i].pc){peer_connection_loop(remotes[i].pc);if(peer_connection_get_state(remotes[i].pc)==PEER_CONNECTION_CONNECTED)++count;}
        atomic_store(&linked,count);
        if(!media_live && atomic_load(&received)>0){media_live=true;call_set_status("Conference audio is flowing. Use app controls to mute/unmute.");}
        if(atomic_load(&muted)!=old_mute){old_mute=atomic_load(&muted);snprintf(path,sizeof(path),"/ocs/v2.php/apps/spreed/api/v4/call/%s?format=json",room);data=call_http("PUT",path,old_mute?"flags=1":"flags=3",error,sizeof(error));if(data){cJSON_Delete(data);data=NULL;}else break;}
        if(!publisher_live && time(NULL)>connect_deadline){snprintf(error,sizeof(error),"Audio connection timed out. Check HPB/Janus UDP media access.");break;}
        mic_update(&mic);play_audio();
        if(!mic.capturing){snprintf(error,sizeof(error),"Microphone stopped: %.200s",mic.status);break;}
        svcSleepThread(2000000);
    }
end:
    if(error[0] && !atomic_load(&stopping))call_set_status(error);
    cleanup=true;call_http_cleanup_mode();
    mic_set_sink(NULL,NULL);mic_stop(&mic);
    if(audio_open){audoutStopAudioOut();audoutExit();audio_open=false;}
    close_peer(&publisher);for(unsigned i=0;i<REMOTES;i++)close_peer(&remotes[i]);
    if(encoder){opus_encoder_destroy(encoder);encoder=NULL;}
    if(peer_open){peer_deinit();peer_open=false;}
    if(in_call){snprintf(path,sizeof(path),"/ocs/v2.php/apps/spreed/api/v4/call/%s?format=json",room);data=call_http("DELETE",path,NULL,error,sizeof(error));cJSON_Delete(data);data=NULL;in_call=false;}
    if(in_room){snprintf(path,sizeof(path),"/ocs/v2.php/apps/spreed/api/v4/room/%s/participants/active?format=json",room);data=call_http("DELETE",path,NULL,error,sizeof(error));cJSON_Delete(data);data=NULL;in_room=false;}
    call_ws_close();call_http_close();cJSON_Delete(settings);settings=NULL;cJSON_Delete(pending_events);pending_events=NULL;cJSON_Delete(answer);cJSON_Delete(data);
    call_erase(&account,sizeof(account));memset(nc_session,0,sizeof(nc_session));memset(encode_samples,0,sizeof(encode_samples));memset(decode_samples,0,sizeof(decode_samples));
    cleanup=false;atomic_store(&finished,true);
}
static Thread engine_thread;
static bool engine_open;
void call_engine_init(void){
 mutexInit(&state_guard);atomic_init(&stopping,false);atomic_init(&finished,true);atomic_init(&muted,false);
 atomic_init(&sent,0);atomic_init(&received,0);atomic_init(&linked,0);engine_open=false;
 call_set_status("Ready to call.");
}
Result call_engine_start(const CallRequest *start){
 if(!call_validate_request(start))return MAKERESULT(345,1);
 if(engine_open){
  if(!atomic_load(&finished))return MAKERESULT(345,2);
  threadWaitForExit(&engine_thread);threadClose(&engine_thread);engine_open=false;
 }
 memset(&account,0,sizeof(account));
 snprintf(account.server,sizeof(account.server),"%s",start->server);
 snprintf(account.username,sizeof(account.username),"%s",start->username);
 snprintf(account.app_password,sizeof(account.app_password),"%s",start->password);
 snprintf(room,sizeof(room),"%s",start->room);snprintf(room_title,sizeof(room_title),"%s",start->title);
 atomic_store(&stopping,false);atomic_store(&finished,false);atomic_store(&muted,false);
 atomic_store(&sent,0);atomic_store(&received,0);atomic_store(&linked,0);
 memset(remotes,0,sizeof(remotes));memset(&publisher,0,sizeof(publisher));memset(&mic,0,sizeof(mic));
 microphone[0]=0;encode_used=0;audio_open=peer_open=in_call=in_room=false;cleanup=false;
 call_set_status("Connecting to call...");
 const int core=-2;
 Result rc=threadCreate(&engine_thread,run_call,NULL,NULL,512*1024,0x2c,core);
 if(R_SUCCEEDED(rc)){rc=threadStart(&engine_thread);if(R_FAILED(rc))threadClose(&engine_thread);}
 if(R_FAILED(rc)){atomic_store(&finished,true);call_erase(&account,sizeof(account));call_set_status("Unable to start call worker");return rc;}
 engine_open=true;return 0;
}
void call_engine_status(CallState *out){
 memset(out,0,sizeof(*out));out->version=CALL_VERSION;out->active=engine_open && !atomic_load(&finished);
 out->muted=atomic_load(&muted);out->sent=atomic_load(&sent);out->received=atomic_load(&received);out->linked=atomic_load(&linked);
 snprintf(out->title,sizeof(out->title),"%s",room_title);
 mutexLock(&state_guard);snprintf(out->status,sizeof(out->status),"%s",status);mutexUnlock(&state_guard);
}
void call_engine_mute(void){if(engine_open && !atomic_load(&finished))atomic_store(&muted,!atomic_load(&muted));}
void call_engine_stop(void){if(engine_open && !atomic_load(&finished))atomic_store(&stopping,true);}

void call_engine_join(void){if(engine_open){threadWaitForExit(&engine_thread);threadClose(&engine_thread);engine_open=false;}}
