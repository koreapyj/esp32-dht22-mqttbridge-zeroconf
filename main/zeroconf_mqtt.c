#include <stdio.h>

#include "mdns.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

static char *mqtt_uri = NULL;
static const char *TAG = "zeroconf_mqtt";
static esp_mqtt_client_handle_t client = NULL;

static void task_zeroconf_mqtt(void *_);

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == MQTT_EVENT_DISCONNECTED || event_id == MQTT_EVENT_ERROR) {
        xTaskCreate(task_zeroconf_mqtt, "task_zeroconf_mqtt", 4096, NULL, 3, NULL);
    }
}

static void zeroconf_mqtt_client_init() {
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = mqtt_uri,
        .skip_cert_common_name_check = true
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, event_handler, NULL);
    esp_mqtt_client_start(client);
}

static void task_zeroconf_mqtt(void *_) {
    mdns_result_t *result = NULL;
    mdns_ip_addr_t *a = NULL;
    while(1) {
        if(mdns_query_ptr("_mqtt", "_tcp", 5000, 1,  &result)) {
            ESP_LOGW(TAG, "mdns_query_ptr returned non zero code");
            vTaskDelay(1000);
            continue;
        }

        if(!result || !(a=result->addr)) {
            ESP_LOGI(TAG, "no result");
            vTaskDelay(1000);
            continue;
        }

        free(mqtt_uri);
        if(a->addr.type == IPADDR_TYPE_V6){
            asprintf(&mqtt_uri, "mqtt://[" IPV6STR "]:%d", IPV62STR(a->addr.u_addr.ip6), result->port);
        } else {
            asprintf(&mqtt_uri, "mqtt://" IPSTR ":%d", IP2STR(&(a->addr.u_addr.ip4)), result->port);
        }
        break;
    }
    ESP_LOGI(TAG, "%s", mqtt_uri);

    mdns_query_results_free(result);

    zeroconf_mqtt_client_init();
    vTaskDelete(NULL);
}

esp_err_t zeroconf_mqtt_publish(const char *topic, const char *data, int len, int qos, int retain) {
    return esp_mqtt_client_publish(client, topic, data, len, qos, retain);
}

esp_err_t zeroconf_mqtt_init(void) {
    xTaskCreate(task_zeroconf_mqtt, "task_zeroconf_mqtt", 4096, NULL, 3, NULL);

    return ESP_OK;
}
