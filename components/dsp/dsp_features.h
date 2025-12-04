#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

float dsp_compute_rms(const int32_t *samples, size_t count);
float dsp_compute_spectral_centroid_fft(const int32_t *samples, size_t count, int sample_rate);
void dsp_apply_gain(int32_t *samples, size_t count, float gain);


#ifdef __cplusplus
}
#endif
