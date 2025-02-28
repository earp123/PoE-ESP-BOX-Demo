
/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>


#include "bsp_board.h"
#include "bsp/esp-bsp.h"
#include <device_driver.h>
#include "esp_check.h"
#include "app_wifi.h"
#include "app_sr.h"
#include "rmaker_devices.h"
#include "settings.h"
#include "app_led.h"
#include "ui_main.h"
#include "ui_net_config.h"

#include "esp_eth.h"
#include "mqtt_client.h"
#include "imeg_mqtt.h"
#include "app_ethernet.h"


static const char *TAG = "imeg_mqtt";

esp_mqtt_client_handle_t imeg_mqtt_client;

//static bool g_is_connected = 0;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}


/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "wakeup", "Starting up...", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/color_hsv", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        // msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        // ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_task(void *args)
{

    ESP_LOGI(TAG, "mqtt task started~~~~~~~~~~~~~~~~~~");
    /* 
        Initialize Wi-Fi. 
        Note that, this should be called before esp_mqtt_client_init()
     */
    //app_wifi_init();
   app_ethernet_init();

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://192.168.1.18:1883",
    };

    imeg_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(imeg_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    /* Start the Wi-Fi. */
    // esp_err_t err = app_wifi_start();
    // if (err != ESP_OK) {
    //     ESP_LOGE(TAG, "Could not start Wifi");
    // }

    app_ethernet_start();

    /* Start the MQTT client*/
    esp_mqtt_client_start(imeg_mqtt_client);

    vTaskDelete(NULL);
}

void app_mqtt_get_client(esp_mqtt_client_handle_t *client_p)
{
    *client_p = imeg_mqtt_client;
}

void app_mqtt_start(void)
{
    //bsp_btn_register_callback(BSP_BUTTON_CONFIG, BUTTON_LONG_PRESS_START, wifi_credential_reset, NULL);

    BaseType_t ret_val = xTaskCreatePinnedToCore(mqtt_task, "MQTT Task", 6 * 1024, NULL, 1, NULL, 0);
    ESP_ERROR_CHECK_WITHOUT_ABORT((pdPASS == ret_val) ? ESP_OK : ESP_FAIL);
}
