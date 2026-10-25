#include "pcm.h"
unsigned pcm_peak_percent(const int16_t *samples,size_t count){
    unsigned peak=0;
    for(size_t i=0;i<count;i++){int v=samples[i];unsigned a=(unsigned)(v<0?-v:v);if(a>peak)peak=a;}
    return (peak*100u+32767u)/32768u;
}
void pcm_test_tone(int16_t *samples,size_t frames,unsigned channels,unsigned rate){
    for(size_t i=0;i<frames;i++){
        int16_t value=((i*880u/rate)&1)?600:-600;
        for(unsigned c=0;c<channels;c++)samples[i*channels+c]=value;
    }
}
