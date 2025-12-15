#pragma once
#include "esp_err.h"
#include "esp_check.h"

/**
 * @brief Initialize WiFi in SRA mode and block until connected or failed.
 * 
 * @return ESP_OK on success, ESP_FAIL on failure. 
 */
esp_err_t wifi_manager_init(void);
