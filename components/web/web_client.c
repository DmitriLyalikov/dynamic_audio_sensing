/**
 * @file web_client.c
 * @author Dmitri Lyalikov (dvl2013@nyu.edu)
 * @brief Implements the web client task that sends audio frames over WebSocket
 * @version 0.1
 * @date 2025-12-15
 */

#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "audio_frame.h"
#include "websocket_server.h"

static const char *TAG = "web_client";

// External queue handle                                
extern QueueHandle_t audio_frame_queue;

// WebSocket packet format                                  
/*
 * [Header]
 *  uint32_t magic
 *  uint32_t sample_count
 *  float    rms
 *  float    centroid
 *  float    gain
 *  uint8_t  scene
 *
 * [Payload]
 *  int16_t samples_in[sample_count]
 *  int16_t samples_out[sample_count]
 */

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t sample_count;
    float rms;
    float centroid;
    float gain;
    uint8_t scene;
} ws_audio_header_t;

// Serializatio
static size_t serialize_audio_frame(
    const audio_frame_t *frame,
    uint8_t *out_buf,
    size_t buf_size)
{
    size_t header_size = sizeof(ws_audio_header_t);
    size_t audio_bytes = frame->sample_count * sizeof(int16_t);
    size_t total_size  = header_size + 2 * audio_bytes;

    if (buf_size < total_size) {
        return 0;
    }

    ws_audio_header_t hdr = {
        .magic        = frame->magic,
        .sample_count = frame->sample_count,
        .rms          = frame->rms,
        .centroid     = frame->centroid,
        .gain         = frame->gain,
        .scene        = (uint8_t)frame->scene
    };

    uint8_t *p = out_buf;
    memcpy(p, &hdr, header_size);
    p += header_size;

    memcpy(p, frame->samples_in, audio_bytes);
    p += audio_bytes;

    memcpy(p, frame->samples_out, audio_bytes);

    return total_size;
}

// Web client task                                  
void web_client_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Web client task started");

    // Static transmit buffer
    static uint8_t tx_buffer[4096];

    while (1) {
        audio_frame_t *frame = NULL;

        // Wait for processed audio frame
        if (xQueueReceive(audio_frame_queue, &frame, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        if (!frame || frame->magic != AUDIO_FRAME_MAGIC) {
            ESP_LOGW(TAG, "Invalid audio frame received");
            goto cleanup;
        }

        // Serialize frame      
        size_t pkt_len = serialize_audio_frame(
            frame, tx_buffer, sizeof(tx_buffer));

        if (pkt_len == 0) {
            ESP_LOGW(TAG, "WebSocket packet too large, dropping frame");
            goto cleanup;
        }

        // Send over WebSocket
        ws_server_send_bin_all((char *)tx_buffer, pkt_len);

cleanup:
        // Free frame + buffers
        if (frame) {
            free(frame->samples_in);
            free(frame->samples_out);
            free(frame);
        }
    }
}
