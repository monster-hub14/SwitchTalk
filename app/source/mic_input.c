/* Captures the selected real input device; no audio output or files. */
#include "mic_input.h"
#include "pcm.h"
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static AudioInBuffer buffers[2];
static bool opened;
static size_t chunk_size;
static u64 last_buffer;
static void (*sample_sink)(const int16_t*,size_t,unsigned,unsigned,void*);
static void *sink_ctx;
void mic_set_sink(void (*sink)(const int16_t*,size_t,unsigned,unsigned,void*),void *ctx){sample_sink=sink;sink_ctx=ctx;}
static void close_input(MicState *s){
    if(opened){audinStopAudioIn();audinExit();opened=false;}
    s->capturing=false;
    for(int i=0;i<2;i++)if(buffers[i].buffer){
        memset(buffers[i].buffer,0,buffers[i].buffer_size);free(buffers[i].buffer);buffers[i].buffer=NULL;
    }
}
void mic_stop(MicState *s){close_input(s);}
static void fail(MicState *s,const char *op,Result rc){
    snprintf(s->status,sizeof(s->status),"%s failed: 0x%08x. Press X to rescan/retry.",op,rc);close_input(s);
}
void mic_select(MicState *s,unsigned selected){
    close_input(s);s->selected=selected;s->blocks=s->level=s->peak=s->raw_peak=0;
    if(selected>=s->device_count){snprintf(s->status,sizeof(s->status),"No input devices listed. Connect a mic and press X.");return;}
    Result rc=audinInitialize();
    if(R_FAILED(rc)){fail(s,"Open input service",rc);return;}
    opened=true;
    /* libnx opens BuiltInHeadset during initialization. Close that session before
       opening the selected device. The default buffer event is unused here:
       completion is polled directly on the new IAudioIn session. */
    serviceClose(audinGetServiceSession_AudioIn());
    char name[256]={0};snprintf(name,sizeof(name),"%s",s->devices[selected]);
    PcmFormat fmt;AudioInState state;
    rc=audinOpenAudioIn(name,s->active,48000,2u<<16,&s->rate,&s->channels,&fmt,&state);
    if(R_FAILED(rc)){fail(s,"Open selected microphone",rc);return;}
    s->active[255]=0;
    if(fmt!=PcmFormat_Int16 || s->rate<8000 || s->rate>192000 || !s->channels || s->channels>2){
        snprintf(s->status,sizeof(s->status),"Unsupported input format: %u Hz, %u channels, PCM %d",s->rate,s->channels,fmt);close_input(s);return;
    }
    chunk_size=(s->rate/20)*s->channels*sizeof(int16_t);
    size_t allocated=(chunk_size+4095)&~(size_t)4095;
    memset(buffers,0,sizeof(buffers));
    for(int i=0;i<2;i++){
        buffers[i].buffer=memalign(4096,allocated);buffers[i].buffer_size=allocated;buffers[i].data_size=chunk_size;
        if(!buffers[i].buffer){snprintf(s->status,sizeof(s->status),"Unable to allocate microphone buffers");close_input(s);return;}
        memset(buffers[i].buffer,0,allocated);
        rc=audinAppendAudioInBuffer(&buffers[i]);
        if(R_FAILED(rc)){fail(s,"Queue microphone capture",rc);return;}
    }
    rc=audinStartAudioIn();
    if(R_FAILED(rc)){fail(s,"Start microphone capture",rc);return;}
    s->capturing=true;last_buffer=armGetSystemTick();
    snprintf(s->status,sizeof(s->status),"Listening to microphone input. Speak to move the meter.");
}
void mic_rescan(MicState *s){
    close_input(s);memset(s,0,sizeof(*s));
    Result rc=audinInitialize();
    if(R_FAILED(rc)){fail(s,"Enumerate microphone inputs",rc);return;}
    opened=true;u32 count=0;
    rc=audinListAudioIns(&s->devices[0][0],MIC_DEVICE_MAX,&count);
    close_input(s);
    if(R_FAILED(rc)){fail(s,"List microphone inputs",rc);return;}
    s->device_count=count>MIC_DEVICE_MAX?MIC_DEVICE_MAX:count;
    for(unsigned i=0;i<s->device_count;i++)s->devices[i][255]=0;
    if(!s->device_count){
        /* Some firmware lists no inputs until a device is attached. Opening the
           known headset name can still return silence; never claim detection. */
        snprintf(s->devices[0],256,"BuiltInHeadset");s->device_count=1;
    }
    /* Prefer an advertised USB input when present; left/right selects any input. */
    unsigned selected=0;
    for(unsigned i=0;i<s->device_count;i++)
        if(strstr(s->devices[i],"Uac") || strstr(s->devices[i],"Usb") || strstr(s->devices[i],"USB")){selected=i;break;}
    mic_select(s,selected);
}
void mic_update(MicState *s){
    if(!s->capturing)return;
    if(armTicksToNs(armGetSystemTick()-last_buffer)>8000000000ULL){
        snprintf(s->status,sizeof(s->status),"No capture buffers received. Connect/select a microphone, then press X.");close_input(s);return;
    }
    for(int i=0;i<4;i++){
        AudioInBuffer *done=NULL;u32 count=0;
        Result rc=audinGetReleasedAudioInBuffer(&done,&count);
        if(R_FAILED(rc)){fail(s,"Read microphone input",rc);return;}
        if(!count || !done)return;
        size_t n=done->data_size;if(n>chunk_size)n=chunk_size;
        n-=n%(s->channels*sizeof(int16_t));
        if(n){last_buffer=armGetSystemTick();++s->blocks;}
        s->level=pcm_peak_percent(done->buffer,n/sizeof(int16_t));
        unsigned raw=0;const int16_t *samples=done->buffer;
        for(size_t j=0;j<n/sizeof(int16_t);j++){int v=samples[j];unsigned a=(unsigned)(v<0?-v:v);if(a>raw)raw=a;}
        if(raw>s->raw_peak)s->raw_peak=raw;
        if(s->level>s->peak)s->peak=s->level;
        snprintf(s->status,sizeof(s->status),"%s",raw?"Non-zero input received. Speak and watch the level.":"Capture is running, but samples are silent. Check mic mute/input selection.");
        if(sample_sink && n)sample_sink(done->buffer,n/sizeof(int16_t)/s->channels,s->rate,s->channels,sink_ctx);
        memset(done->buffer,0,done->buffer_size);done->data_size=chunk_size;
        rc=audinAppendAudioInBuffer(done);
        if(R_FAILED(rc)){fail(s,"Requeue microphone input",rc);return;}
    }
}
