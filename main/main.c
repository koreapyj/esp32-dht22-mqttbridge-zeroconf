#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "dht.h"
#include "mdns.h"
#include "mqtt_client.h"

#include "smartconfig_wifi_init.h"
#include "zeroconf_mqtt.h"

#define MQTT_TOPIC_STATE CONFIG_MQTT_PREFIX "sensor/" CONFIG_MQTT_SENSOR_NAME
#define MQTT_TOPIC_AVAILABILITY CONFIG_MQTT_PREFIX "sensor/" CONFIG_MQTT_SENSOR_NAME "/availability"
#define MQTT_TOPIC_CONFIG CONFIG_MQTT_PREFIX "sensor/" CONFIG_MQTT_SENSOR_NAME "/config"

static const char *TAG = "esp32_mqtt_dht22";

static void task_dht22(void*_) {
    int16_t humidity = 0, temperature = 0;
    char *data = NULL;
    int data_len = 0;
    esp_err_t dht_error = 0;

    char data_config[] = "{\"unique_id\":\"" CONFIG_MQTT_SENSOR_NAME "\",\"avty_t\":\"" MQTT_TOPIC_AVAILABILITY "\",\"stat_t\":\"" MQTT_TOPIC_STATE "\",\"json_attr_t\":\"" MQTT_TOPIC_STATE "\"" "}";

    while(1) {
        vTaskDelay(5000 / portTICK_PERIOD_MS);

        dht_error = dht_read_data(DHT_TYPE_AM2301, 0x4, &humidity, &temperature);
        ESP_LOGI(TAG, "return: %d, temperature: %d.%1d, humidity:%d.%1d", dht_error, (int) (temperature/10), (int) (temperature%10), (int) (humidity/10), (int) (humidity%10));
        if(dht_error != ESP_OK) continue;

        free(data);

        data_len = asprintf(&data, "{\"temperature\":%d.%1d,\"humidity\":%d.%1d}", (int) (temperature/10), (int) (temperature%10), (int) (humidity/10), (int) (humidity%10));
        if(data_len == -1) continue;

        dht_error = zeroconf_mqtt_publish(MQTT_TOPIC_STATE, data, data_len, 0, 0);
        if(dht_error != ESP_OK) {
            ESP_LOGE(TAG, "mqtt publish failed");
            continue;
        }

        dht_error = zeroconf_mqtt_publish(MQTT_TOPIC_AVAILABILITY, "online", sizeof("online") - 1, 0, 0);
        if(dht_error != ESP_OK) {
            continue;
        }

        dht_error = zeroconf_mqtt_publish(MQTT_TOPIC_CONFIG, data_config, sizeof(data_config) - 1, 0, 0);
        if(dht_error != ESP_OK) {
            continue;
        }

        ESP_LOGI(TAG, "%s %s", MQTT_TOPIC_STATE, data);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK( esp_netif_init() );
    ESP_ERROR_CHECK( esp_event_loop_create_default() );

    ESP_ERROR_CHECK( smartconfig_wifi_init() );
    ESP_ERROR_CHECK( mdns_init() );
    ESP_ERROR_CHECK( zeroconf_mqtt_init() );

    xTaskCreate(task_dht22, "task_dht22", 4096, NULL, 3, NULL);
}
