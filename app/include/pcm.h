#ifndef SWITCHTALK_PCM_H
#define SWITCHTALK_PCM_H
#include <stdint.h>
#include <stddef.h>
unsigned pcm_peak_percent(const int16_t *samples,size_t count);
void pcm_test_tone(int16_t *samples,size_t frames,unsigned channels,unsigned rate);
#endif
