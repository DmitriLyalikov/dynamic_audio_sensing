#include "dsp_features.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "esp_dsp.h"

float dsp_compute_rms(const int32_t *samples, size_t count) {
    if (!samples || count == 0) return 0.0f;

    double sum_sq = 0.0;
    for (size_t i = 0; i < count; ++i) {
        int32_t sample = samples[i] >> 8;  // Convert 32‑bit MSB padded to 24‑bit
        sum_sq += (double)sample * sample;
    }

    double mean_sq = sum_sq / count;
    double rms = sqrt(mean_sq);

    return (float)(rms / (float)(1 << 23));  // Normalize to [-1,1]
}

float dsp_compute_spectral_centroid_fft(const int32_t *samples, size_t count, int sample_rate) {
    if (!samples || count == 0) {
        return 0.0f;
    }

    // ESP‑DSP expects float real input. Prepare buffer of length count.
    float *buf = malloc(sizeof(float) * count);
    if (!buf) return 0.0f;

    // Convert from 32-bit signed int to float (shift down 8 bits if using 24-bit mic data)
    for (size_t i = 0; i < count; i++) {
        buf[i] = (float)(samples[i] >> 8);
    }

    // Prepare FFT: real → complex radix‑2
    // We need buffer of length 2*count: [Re0, Im0, Re1, Im1, ...]
    float *fft_buf = malloc(sizeof(float) * 2 * count);
    if (!fft_buf) {
        free(buf);
        return 0.0f;
    }
    // Fill complex buffer: real = buf, imag = 0
    for (size_t i = 0; i < count; i++) {
        fft_buf[2*i + 0] = buf[i];
        fft_buf[2*i + 1] = 0.0f;
    }

    // Initialize FFT tables (only once ideally — but safe to call)
    dsps_fft2r_init_fc32(NULL, count);

    // Perform FFT in-place on fft_buf
    dsps_fft2r_fc32_ae32(fft_buf, count);
    dsps_bit_rev_fc32_ansi(fft_buf, count);

    // Now fft_buf contains complex spectrum: [Re0, Im0, Re1, Im1, ...]
    // Compute magnitude and spectral centroid
    float total_mag = 0.0f;
    float centroid = 0.0f;
    size_t half = count / 2;  // Nyquist

    for (size_t k = 1; k < half; k++) {
        float re = fft_buf[2*k];
        float im = fft_buf[2*k + 1];
        float mag = sqrtf(re*re + im*im);
        float freq = ((float)k * sample_rate) / count;
        centroid += freq * mag;
        total_mag += mag;
    }

    float result = (total_mag > 0.0f) ? (centroid / total_mag) : 0.0f;

    free(buf);
    free(fft_buf);

    return result;
}

void dsp_apply_gain(int32_t *samples, size_t count, float gain) {
    for (size_t i = 0; i < count; i++) {
        int64_t scaled = (int64_t)(samples[i] * gain);
        if (scaled > INT32_MAX) scaled = INT32_MAX;
        if (scaled < INT32_MIN) scaled = INT32_MIN;
        samples[i] = (int32_t)scaled;
    }
}