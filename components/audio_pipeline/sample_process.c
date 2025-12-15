/**
 * @file sample_process.c
 * @author Dmitri Lyalikov (dvl2013@nyu.edu)
 * @brief Implements the sample processing task that reads from the microphone,
 *        performs DSP feature extraction and classification, and sends audio frames to queue.
 * @version 0.1
 * @date 2025-12-15
 */


#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_heap_caps.h"

#include "mic_input.h"
#include "dsp_features.h"
#include "audio_frame.h"    
#include "sdkconfig.h"


#define SAMPLE_RATE     16000
#define SAMPLE_COUNT    512

#define RMS_QUIET_TH    0.025f
#define RMS_NOISE_TH    0.10f

#define CENTROID_MIN    600.0f
#define CENTROID_MAX    3200.0f

#define GAIN_QUIET      3.0f
#define GAIN_SPEECH     1.0f
#define GAIN_NOISE      0.5f

static const char *TAG = "sample_process";

// External queue handle                                
// Defined and created in main.c 
extern QueueHandle_t audio_frame_queue;

// Utility functions                                   
static inline int16_t clamp_int16(int32_t x)
{
    if (x > 32767) return 32767;
    if (x < -32768) return -32768;
    return (int16_t)x;
}

static inline int16_t convert_32_to_16(int32_t x)
{
    // INMP441: 24-bit left-aligned in 32-bit word
    return clamp_int16(x >> 8);
}

// Processing Task                                    
void sample_process_task(void *arg)
{
    ESP_LOGI(TAG, "Initializing microphone input...");
    mic_input_init();

    // Allocate persistent buffers
    int32_t *raw_buf  = heap_caps_malloc(
        SAMPLE_COUNT * sizeof(int32_t), MALLOC_CAP_8BIT);
    int32_t *proc_buf = heap_caps_malloc(
        SAMPLE_COUNT * sizeof(int32_t), MALLOC_CAP_8BIT);

    int16_t *in16_buf  = heap_caps_malloc(
        SAMPLE_COUNT * sizeof(int16_t), MALLOC_CAP_8BIT);
    int16_t *out16_buf = heap_caps_malloc(
        SAMPLE_COUNT * sizeof(int16_t), MALLOC_CAP_8BIT);

    if (!raw_buf || !proc_buf || !in16_buf || !out16_buf) {
        ESP_LOGE(TAG, "Buffer allocation failed");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Sample processing task started");

    while (1) {
        // 1. Acquire audio samples                                           
        size_t n = mic_input_read(raw_buf, SAMPLE_COUNT);
        if (n != SAMPLE_COUNT) {
            ESP_LOGW(TAG, "Short read: %d samples", n);
            continue;
        }

        memcpy(proc_buf, raw_buf, SAMPLE_COUNT * sizeof(int32_t));

        // 2. Feature extraction                                             
        float rms = dsp_compute_rms(raw_buf, SAMPLE_COUNT);
        float centroid = 0.0f;

        if (rms > 1e-6f) {
            centroid = dsp_compute_spectral_centroid_fft(
                raw_buf, SAMPLE_COUNT, SAMPLE_RATE);
        }

        // 3. Scene classification                                           
        audio_scene_t scene;
        float gain;

        if (rms < RMS_QUIET_TH) {
            scene = SCENE_QUIET;
            gain  = GAIN_QUIET;
        }
        else if (rms <= RMS_NOISE_TH &&
                 centroid >= CENTROID_MIN &&
                 centroid <= CENTROID_MAX) {
            scene = SCENE_SPEECH;
            gain  = GAIN_SPEECH;
        }
        else {
            scene = SCENE_NOISE;
            gain  = GAIN_NOISE;
        }

        // 4. Apply gain                                                     
        dsp_apply_gain(proc_buf, SAMPLE_COUNT, gain);

        // 5. Convert to int16 for transport / playback                        
        for (size_t i = 0; i < SAMPLE_COUNT; i++) {
            in16_buf[i]  = convert_32_to_16(raw_buf[i]);
            out16_buf[i] = convert_32_to_16(proc_buf[i]);
        }

        // 6. Package frame                                                   
        audio_frame_t *frame = malloc(sizeof(audio_frame_t));
        if (!frame) {
            ESP_LOGE(TAG, "Failed to allocate audio_frame");
            continue;
        }

        frame->magic        = AUDIO_FRAME_MAGIC;
        frame->sample_count = SAMPLE_COUNT;
        frame->rms          = rms;
        frame->centroid     = centroid;
        frame->gain         = gain;
        frame->scene        = scene;
        frame->samples_in   = in16_buf;
        frame->samples_out  = out16_buf;

        // 7. Send to downstream consumer                                    
        if (xQueueSend(audio_frame_queue, &frame, 0) != pdTRUE) {
            // Drop frame if consumer is slow 
            free(frame);
        }
    }
}
