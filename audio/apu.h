#ifndef TBA_APU_H
#define TBA_APU_H
#include <stdint.h>
#define TBA_RATE 16000
/* Initial implementation samples register writes once per video frame. */
typedef struct {
    uint32_t phase, step;
    uint16_t frequency, shadow_frequency, length;
    uint8_t enabled, volume, envelope_ticks, envelope_period, envelope_up;
} TbaChannel;
typedef struct {
    TbaChannel channel[4];
    uint8_t regs[0x30], wave[16], sequencer, sweep_ticks, sweep_period, sweep_shift, sweep_down;
    uint16_t tick, lfsr;
    uint32_t sample_clock;
    uint16_t rate;
    int32_t dc;
} TbaApu;
void tba_apu_reset(TbaApu *a);
void tba_apu_set_rate(TbaApu *a,uint16_t rate);
void tba_apu_snapshot(TbaApu *a, const uint8_t regs[0x30], const uint8_t wave[16], uint8_t status);
void tba_apu_render(TbaApu *a,int16_t *out,unsigned int n);
int16_t tba_apu_sample(TbaApu *a);
unsigned int tba_apu_frame_samples(TbaApu *a);
#endif
