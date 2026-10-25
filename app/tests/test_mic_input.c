#include "mic_input.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static Service input_service;
static bool running,fail_init,fail_queue,invalid_format;
static int injected;
static u64 ticks;
static AudioInBuffer *queue[2];static unsigned queued;
static char selected[256];
Result audinInitialize(void){if(fail_init)return 0x55;input_service.open=1;return 0;}
void audinExit(void){input_service.open=0;queued=0;running=false;}
Service *audinGetServiceSession_AudioIn(void){return &input_service;}
void serviceClose(Service *s){s->open=0;}
Result audinOpenAudioIn(const char *name,char *out,u32 rate,u32 channels,u32 *actual_rate,u32 *actual_channels,PcmFormat *format,AudioInState *state){
    assert(!input_service.open && channels==(2u<<16));input_service.open=1;
    strcpy(selected,name);strcpy(out,name);*actual_rate=48000;
    *actual_channels=strstr(name,"Uac")?1:2;*format=invalid_format?PcmFormat_Invalid:PcmFormat_Int16;*state=AudioInState_Stopped;return 0;
}
Result audinStartAudioIn(void){assert(input_service.open && queued==2);running=true;return 0;}
Result audinStopAudioIn(void){running=false;return 0;}
Result audinListAudioIns(char *names,s32 count,u32 *total){
    assert(count>=2);strcpy(names,"BuiltInHeadset");strcpy(names+256,"UacIn");*total=2;return 0;
}
Result audinAppendAudioInBuffer(AudioInBuffer *b){
    if(fail_queue)return 0x66;
    assert(queued<2 && !(b->buffer_size%4096) && b->data_size<=b->buffer_size);
    queue[queued++]=b;return 0;
}
Result audinGetReleasedAudioInBuffer(AudioInBuffer **b,u32 *count){
    if(!injected){*count=0;*b=NULL;return 0;}
    assert(running && queued);*b=queue[0];queue[0]=queue[1];--queued;
    int16_t *data=(*b)->buffer;for(unsigned i=0;i<(*b)->data_size/2;i++)data[i]=injected==1?10000:0;
    injected=0;*count=1;return 0;
}
u64 armGetSystemTick(void){return ticks;}
u64 armTicksToNs(u64 t){return t;}
int main(void){
    MicState s={0};mic_rescan(&s);
    assert(s.capturing && s.device_count==2 && s.selected==1 && !strcmp(selected,"UacIn") && s.channels==1);
    injected=1;mic_update(&s);assert(s.blocks==1 && s.raw_peak==10000 && s.level==31);
    injected=2;mic_update(&s);assert(s.blocks==2 && s.level==0 && s.peak==31 && strstr(s.status,"silent"));
    mic_select(&s,0);assert(s.capturing && s.channels==2 && !strcmp(selected,"BuiltInHeadset"));
    ticks+=9000000000ULL;mic_update(&s);assert(!s.capturing && !running && !input_service.open && strstr(s.status,"No capture buffers"));
    fail_queue=true;mic_select(&s,0);assert(!s.capturing && !input_service.open && strstr(s.status,"00000066"));fail_queue=false;
    invalid_format=true;mic_select(&s,0);assert(!s.capturing && strstr(s.status,"Unsupported input format"));invalid_format=false;
    fail_init=true;mic_rescan(&s);assert(!s.capturing && strstr(s.status,"00000055"));fail_init=false;
    mic_rescan(&s);assert(s.capturing);mic_stop(&s);assert(!s.capturing && !input_service.open);
    puts("Mic capture: USB/3.5mm selection, actual format, nonzero/silent buffers, timeout, failures and cleanup: PASS");return 0;
}
