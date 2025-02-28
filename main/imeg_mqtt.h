#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "mqtt_client.h"

void app_mqtt_get_client(esp_mqtt_client_handle_t *client_p);
void app_mqtt_start(void);


#ifdef __cplusplus
}
#endif