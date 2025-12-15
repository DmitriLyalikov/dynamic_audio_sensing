/**
 * @file mic_input.c
 * @author Dmitri Lyalikov (dvl2013@nyu.edu)
 * @brief Implements microphone input using I2S interface. Uses INMP441 microphone connected on:
 *       BCK  - GPIO26
 *       WS   - GPIO25
 *       DATA - GPIO34
 * 
 * @version 0.1
 * @date 2025-12-15
 */


#include "mic_input.h"
#include "driver/i2s.h"
#include "esp_log.h"

#define I2S_NUM I2S_NUM_0
#define SAMPLE_RATE CONFIG_MIC_INPUT_SAMPLE_RATE
#define I2S_BCK_IO 26
#define I2S_WS_IO 25
#define I2S_DATA_IN_IO 34

#define DMA_BUF_LEN CONFIG_MIC_INPUT_BUFFER_SIZE
#define DMA_BUF_COUNT CONFIG_MIC_INPUT_BUFFER_COUNT

static const char *TAG = "mic_input";

void mic_input_init(void) {
    // I2S configuration
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // INMP441 outputs 24-bit padded to 32-bit
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    // Pin configuration
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_IO,
        .ws_io_num = I2S_WS_IO,
        .data_in_num = I2S_DATA_IN_IO,
        .data_out_num = I2S_PIN_NO_CHANGE
    };

    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM, &pin_config));
    ESP_LOGI(TAG, "I2S mic initialized at %d Hz", SAMPLE_RATE);
}

size_t mic_input_read(int32_t *buffer, size_t samples) {
    size_t bytes_read = 0;
    esp_err_t err = i2s_read(I2S_NUM, buffer, samples * sizeof(int32_t), &bytes_read, portMAX_DELAY);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_read failed: %s", esp_err_to_name(err));
        return 0;
    }
    return bytes_read / sizeof(int32_t); // Return number of samples
}