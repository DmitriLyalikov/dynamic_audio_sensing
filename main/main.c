#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mic_input.h"
#include "dsp_features.h"
#include "esp_log.h"
#include "esp_dsp.h"
#include "sdkconfig.h"

#define SAMPLE_COUNT CONFIG_MIC_INPUT_BUFFER_SIZE
#define SAMPLE_RATE CONFIG_MIC_INPUT_SAMPLE_RATE

#define GAIN_QUIET  (CONFIG_DSP_GAIN_QUIET_X100 / 100.0f)
#define GAIN_SPEECH (CONFIG_DSP_GAIN_SPEECH_X100 / 100.0f)
#define GAIN_NOISE  (CONFIG_DSP_GAIN_NOISE_X100 / 100.0f)


static const char *TAG = "main";

/**
 * @brief Simple scene classifier based on RMS thresholds
 */
const char *classify_scene(float rms, float centroid) {
    if (rms < 0.01f) {
        return "quiet";
    } else if (centroid > 300 && centroid < 3500) {
        return "speech";
    } else {
        return "background noise";
    }
}

void app_main(void) {
    mic_input_init();
    int32_t audio_buffer[SAMPLE_COUNT];

    dsps_fft2r_init_fc32(NULL, SAMPLE_COUNT);  // Initialize FFT

    while (1) {
        size_t got = mic_input_read(audio_buffer, SAMPLE_COUNT);
        if (got == SAMPLE_COUNT) {
            float rms = dsp_compute_rms(audio_buffer, SAMPLE_COUNT);
            float centroid = dsp_compute_spectral_centroid_fft(audio_buffer, SAMPLE_COUNT, SAMPLE_RATE);
            const char *scene = classify_scene(rms, centroid);

            // Adjust gain based on scene
            float gain = 1.0f;
            if (strcmp(scene, "quiet") == 0) {
                gain = GAIN_QUIET;
            } else if (strcmp(scene, "speech") == 0) {
                gain = GAIN_SPEECH;
            } else if (strcmp(scene, "background noise") == 0) {
                gain = GAIN_NOISE;
            }

            dsp_apply_gain(audio_buffer, SAMPLE_COUNT, gain);

            // Now audio_buffer contains gain-adjusted samples
            ESP_LOGI(TAG, "RMS: %.5f | Centroid: %.1f Hz | Scene: %s | Gain: %.1fx",
                    rms, centroid, scene, gain);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
