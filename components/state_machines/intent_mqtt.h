#pragma once
#include "sdkconfig.h"
#include "mqtt_client.h"


typedef struct mqtt_event_handler_resources_t {
  esp_mqtt_client_config_t *p_mqtt_cfg;
  SemaphoreHandle_t *p_ConnectedClientsSemaphore;
  char *p_connected_clients_count;
} mqtt_event_handler_resources_t;


esp_mqtt_client_handle_t * mqtt_startup_routine(SemaphoreHandle_t *p_ConnectedClientsSemaphore, char *p_connect_clients_count);


