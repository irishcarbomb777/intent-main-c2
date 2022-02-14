#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "mqtt_client.h"
#include "esp_log.h"

#include "intent_mqtt.h"


static const char *TAG = "MQTT";

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    mqtt_event_handler_resources_t *p_mqtt_event_handler_resources = (mqtt_event_handler_resources_t*)handler_args;
    esp_mqtt_client_config_t connection_info = *(p_mqtt_event_handler_resources->p_mqtt_cfg);
    SemaphoreHandle_t *p_ConnectedClientsSemaphore = p_mqtt_event_handler_resources->p_ConnectedClientsSemaphore;
    char *p_connected_clients_count = p_mqtt_event_handler_resources->p_connected_clients_count;

    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "sensors/active", "sensor_07", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        if (!strncmp((event->topic), "client_connections/sensor_01", 28))
        {
          if (!strncmp((event->data), "connect", 7))
          {
            printf("Tablet Connected\n");
            (*p_connected_clients_count)++;
          }
          if (!strncmp((event->data), "disconnect", 10))
          {
            printf("Tablet Disconnected\n");
            (*p_connected_clients_count)--;
          }
          xSemaphoreGive(*p_ConnectedClientsSemaphore);
        }
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

/********************************************************
************ Startup Routine Code ***********************
********************************************************/

esp_mqtt_client_handle_t * mqtt_startup_routine(SemaphoreHandle_t *p_ConnectedClientsSemaphore, char *p_connected_clients_count)
{
  // Initialize MQTT Config Struct
  // esp_mqtt_client_config_t *p_mqtt_cfg = (esp_mqtt_client_config_t*)malloc(sizeof(esp_mqtt_client_config_t));
  // p_mqtt_cfg->host = "10.1.10.94";
  // p_mqtt_cfg->port = 1883;
  // p_mqtt_cfg->client_id = "sensor_dev";
  // p_mqtt_cfg->transport = MQTT_TRANSPORT_UNKNOWN;
  // p_mqtt_cfg->skip_cert_common_name_check = true;

  esp_mqtt_client_config_t mqtt_cfg = {
    .host = "10.1.10.94",
    .port = 1883,
    .client_id = "sensor_dev",
    .transport = MQTT_TRANSPORT_UNKNOWN,
    .skip_cert_common_name_check = true,
  };

  // Initialize Event Handler Resources
  mqtt_event_handler_resources_t *p_mqtt_event_handler_resources = (mqtt_event_handler_resources_t*)malloc(sizeof(mqtt_event_handler_resources_t));
  p_mqtt_event_handler_resources->p_mqtt_cfg = &mqtt_cfg;
  p_mqtt_event_handler_resources->p_ConnectedClientsSemaphore = p_ConnectedClientsSemaphore;
  p_mqtt_event_handler_resources->p_connected_clients_count = p_connected_clients_count;
  

  esp_mqtt_client_handle_t *client = (esp_mqtt_client_handle_t*)malloc(sizeof(esp_mqtt_client_handle_t));
  ESP_LOGI(TAG, "After MAlloc");
  *client = esp_mqtt_client_init(&mqtt_cfg);
  ESP_LOGI(TAG, "After Init");
  esp_mqtt_client_register_event(*client, ESP_EVENT_ANY_ID, mqtt_event_handler, p_mqtt_event_handler_resources);
  ESP_LOGI(TAG, "After Register");
  esp_mqtt_client_start(*client);
  ESP_LOGI(TAG, "After Start");
  return client;
}