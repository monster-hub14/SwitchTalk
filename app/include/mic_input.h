#ifndef SWITCHTALK_MIC_INPUT_H
#define SWITCHTALK_MIC_INPUT_H
#include <switch.h>
#define MIC_DEVICE_MAX 8
typedef struct {
    char devices[MIC_DEVICE_MAX][256],active[256],status[240];
    unsigned device_count,selected,rate,channels,level,peak,raw_peak;
    u64 blocks;
    bool capturing;
} MicState;
void mic_set_sink(void (*sink)(const int16_t*,size_t,unsigned,unsigned,void*),void *ctx);
void mic_rescan(MicState *state);
void mic_select(MicState *state,unsigned selected);
void mic_update(MicState *state);
void mic_stop(MicState *state);
#endif
