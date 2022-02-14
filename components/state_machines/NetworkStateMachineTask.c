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

void NetworkStateMachineTask(void *arg)
{
  // Create Logging Tag
  static const char *TAG = "Sensor State Machine Task Log";

  // Create Network State
  NetworkStateEnum_t eNetworkState = NETWORK_INACTIVE;

  // Type case Network State Machine Context from void pointer to Context Pointer
  NetworkStateMachineTaskContext_t *p_ctxNetworkStateMachineTaskContext = (NetworkStateMachineTaskContext_t*)arg;

  // Initialize Networking Resources
  wifi_startup_routine();
  esp_mqtt_client_handle_t *p_mqtt_client = mqtt_startup_routine(
                                              p_ctxNetworkStateMachineTaskContext->p_xConnectedClientsSemaphore, 
                                              p_ctxNetworkStateMachineTaskContext->p_cConnectedClientsCount
                                            );
  // Create Network Active State Context
  NetworkActiveStateContext_t ctxNetworkActiveState = {
    .p_eNetworkState = &eNetworkState,
    .p_mqtt_client = p_mqtt_client,
    .p_xConnectedClientsSemaphore = p_ctxNetworkStateMachineTaskContext->p_xConnectedClientsSemaphore,
    .p_xStartSetSemaphore = p_ctxNetworkStateMachineTaskContext->p_xStartSetSemaphore,
    .p_xDataTransmitQueue = p_ctxNetworkStateMachineTaskContext->p_xDataTransmitQueue,
    .p_xEndSetSemaphore = p_ctxNetworkStateMachineTaskContext->p_xEndSetSemaphore,
    .p_xNetworkInactiveSemaphore = p_ctxNetworkStateMachineTaskContext->p_xNetworkInactiveSemaphore,
    .p_cConnectedClientsCount = p_ctxNetworkStateMachineTaskContext->p_cConnectedClientsCount,
  };

  // Create Network Inactive State Context
  NetworkInactiveStateContext_t ctxNetworkInactiveState = {
    .p_eNetworkState = &eNetworkState,
    .p_mqtt_client = p_mqtt_client,
    .p_xNetworkActiveSemaphore = p_ctxNetworkStateMachineTaskContext->p_xNetworkActiveSemaphore,
  };

  // Clear Network Active/Inactive Semaphore
  xSemaphoreTake(*(p_ctxNetworkStateMachineTaskContext->p_xNetworkInactiveSemaphore), 0);
  xSemaphoreTake(*(p_ctxNetworkStateMachineTaskContext->p_xNetworkActiveSemaphore), 0);
  // Initiate State Machine
  while (1)
  {
    switch(eNetworkState)
    {
      case NETWORK_INACTIVE:
        vNetworkInactiveState(&ctxNetworkInactiveState);
        break;
      case NETWORK_ACTIVE:
        vNetworkActiveState(&ctxNetworkActiveState);
        break;
      default:
        break; 
    }
  }
}
