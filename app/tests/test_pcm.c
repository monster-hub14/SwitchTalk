#include "pcm.h"
#include <assert.h>
#include <stdio.h>
static int16_t samples[48000];
int main(void){
    assert(pcm_peak_percent(samples,0)==0);
    assert(pcm_peak_percent(samples,10)==0);
    samples[0]=-32768;assert(pcm_peak_percent(samples,10)==100);
    samples[0]=32767;assert(pcm_peak_percent(samples,10)==100);
    samples[0]=1;assert(pcm_peak_percent(samples,10)==1);
    pcm_test_tone(samples,24000,2,48000);
    unsigned transitions=0;
    for(unsigned i=0;i<24000;i++){
        assert(samples[i*2]==samples[i*2+1]);
        assert(samples[i*2]==600 || samples[i*2]==-600);
        if(i && samples[i*2]!=samples[(i-1)*2])++transitions;
    }
    assert(transitions==439 && pcm_peak_percent(samples,48000)==2);
    puts("PCM silence, full scale, low signal, stereo and quiet 440 Hz tone: PASS");return 0;
}
