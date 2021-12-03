#ifndef ZEROCONF_MQTT_H_
#define ZEROCONF_MQTT_H_

#include "esp_err.h"

esp_err_t zeroconf_mqtt_init(void);
esp_err_t zeroconf_mqtt_publish(const char *topic, const char *data, int len, int qos, int retain);

#endif /* ZEROCONF_MQTT_H_ */
