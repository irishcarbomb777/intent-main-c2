#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"

#include "sdkconfig.h"

#include "NetworkStateMachineTask.h"
#include "wifi.h"
#include "intent_mqtt.h"

// Define Network Active State Constants

void vNetworkInactiveState(NetworkInactiveStateContext_t *p_ctxNetworkInactiveState)
{
  // Create Logging Tag
  static const char *TAG = "Network Inactive State Log";
  ESP_LOGI(TAG, "Network Inactive State...");
  // Initialize Static Data Buffers

  // Parse out Context Pointers
  NetworkStateEnum_t *p_eNetworkState = p_ctxNetworkInactiveState->p_eNetworkState;
  esp_mqtt_client_handle_t *p_mqtt_client = p_ctxNetworkInactiveState->p_mqtt_client;
  SemaphoreHandle_t *p_xNetworkActiveSemaphore = p_ctxNetworkInactiveState->p_xNetworkActiveSemaphore;

  // Unsubscribe from MQTT Channels
  esp_mqtt_client_unsubscribe(*p_mqtt_client, "client_connections");

  // Set Networking to Low Power Mode
  wifi_enable_modem_sleep();

  while((*p_eNetworkState) == NETWORK_INACTIVE)
  {
    ESP_LOGI(TAG, "Network Inactive State...");
    if (xSemaphoreTake(*p_xNetworkActiveSemaphore, pdMS_TO_TICKS(1000)))
      *p_eNetworkState = NETWORK_ACTIVE;
  }
}