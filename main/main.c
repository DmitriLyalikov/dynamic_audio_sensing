/**
 * @file main.c
 * @author Dmitri Lyalikov (dvl2013@nyu.edu)
 * @brief Implements the top level application of embedded dynamic audio sensing. Configures WiFI, 
 *      starts web server, and audio processing tasks. 
 * @version 0.1
 * @date 2025-12-15
 */


#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "mdns.h"

#include "sdkconfig.h"

#include "wifi_manager.h"
#include "web_server.h"
#include "web_client.h"
#include "audio_frame.h"
#include "websocket_server.h"

// Globals                       

static const char *TAG = "main";

// Shared queue: DSP -> transport 
QueueHandle_t audio_frame_queue;


// Forward declarations
void sample_process_task(void *pvParameters);

// Init helpers
static esp_err_t init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

static void init_mdns(void)
{
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(CONFIG_MDNS_HOSTNAME));
    ESP_LOGI(TAG, "mDNS hostname set to: %s", CONFIG_MDNS_HOSTNAME);

    ESP_ERROR_CHECK(
        mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0)
    );
}

// Main application entry point
void app_main(void)
{
    ESP_LOGI(TAG, "Starting Dynamic Audio Sensing system...");

    // 1. Initialize NVS 
    ESP_ERROR_CHECK(init_nvs());

    // 2. Initialize WiFi (blocking until connected) 
    ESP_ERROR_CHECK(wifi_manager_init());

    // 3. Initialize mDNS
    init_mdns();

    // 4. Create audio frame queue (DSP -> Web)
    audio_frame_queue = xQueueCreate(4, sizeof(audio_frame_t *));
    configASSERT(audio_frame_queue);

    // 5. Start WebSocket server core
    ws_server_start();

    // 6. Start HTTP / WebSocket server task
    xTaskCreate(
        server_task,
        "server_task",
        4096,
        NULL,
        5,
        NULL
    );

    // 7. Log IP address
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        ESP_LOGI(TAG, "ESP32 IP Address: " IPSTR, IP2STR(&ip_info.ip));
    }

    // 8. Start audio processing task (mic + DSP + classification)
    xTaskCreate(
        sample_process_task,
        "sample_process",
        8192,
        NULL,
        6,     // higher priority (real-time)
        NULL
    );

    // 9. Start WebSocket client task (transport only)
    xTaskCreate(
        web_client_task,
        "web_client",
        4096,
        NULL,
        5,
        NULL
    );

    ESP_LOGI(TAG, "System initialization complete.");
}
