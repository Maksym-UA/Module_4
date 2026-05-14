#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "wifi_setup.h"
#include "mqtt.h"

static const char *TAG = "main";

#define LED_GPIO             GPIO_NUM_16
#define PUBLISH_INTERVAL_MS  (10 * 1000)

static void handle_mqtt_message(const char *topic, const char *data)
{
    if (topic == NULL || data == NULL) {
        return;
    }

    if (strcmp(topic, MQTT_COMMANDS) != 0) {
        return;
    }

    esp_mqtt_client_handle_t client = mqtt_get_client();

    if (strcmp(data, "ON") == 0) {
        ESP_LOGI(TAG, "Command: LED ON");
        if (gpio_set_level(LED_GPIO, 1) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set LED ON");
        }
    } else if (strcmp(data, "OFF") == 0) {
        ESP_LOGI(TAG, "Command: LED OFF");
        if (gpio_set_level(LED_GPIO, 0) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set LED OFF");
        }
    } else if (strcmp(data, "STATUS") == 0) {
        if (client) {
            if (esp_mqtt_client_publish(client, MQTT_STATUS, "ESP32-S3 is running", 0, 0, 0) < 0) {
                ESP_LOGE(TAG, "Failed to publish status");
            } else {
                ESP_LOGI(TAG, "Status sent");
            }
        }
    } else {
        ESP_LOGW(TAG, "Unknown command: %s", data);
    }
}