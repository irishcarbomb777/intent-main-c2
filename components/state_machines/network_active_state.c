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
#define SENSOR_ID "sensor_01"
#define WATERMARK_THRESHOLD 300
#define MEMCPY_LENGTH (WATERMARK_THRESHOLD*7)
#define TRANSMIT_BUFFER_LENGTH ((WATERMARK_THRESHOLD*7)+1)

// Declare Static Function Prototypes
static void transmit_data_packet_to_mothership(esp_mqtt_client_handle_t *p_mqtt_client, char *p_cCore0TransmitDataBufferAddress);

void vNetworkActiveState(NetworkActiveStateContext_t *p_ctxNetworkActiveState)
{
  // Create Logging Tag
  static const char *TAG = "Network Active State Log";
  // Initialize Static Data Buffers
  static BaseType_t xDataTransmitResult;
  static BaseType_t xNetworkInactiveResult;
  static char *p_cCore0TransmitDataBufferAddress;
  
  // Parse out Context Pointers
  NetworkStateEnum_t *p_eNetworkState = p_ctxNetworkActiveState->p_eNetworkState;
  esp_mqtt_client_handle_t *p_mqtt_client = p_ctxNetworkActiveState->p_mqtt_client;
  SemaphoreHandle_t *p_xConnectedClientsSemaphore = p_ctxNetworkActiveState->p_xConnectedClientsSemaphore;
  SemaphoreHandle_t *p_xStartSetSemaphore = p_ctxNetworkActiveState->p_xStartSetSemaphore;
  SemaphoreHandle_t *p_xEndSetSemaphore = p_ctxNetworkActiveState->p_xEndSetSemaphore;
  SemaphoreHandle_t *p_xNetworkInactiveSemaphore = p_ctxNetworkActiveState->p_xNetworkInactiveSemaphore;
  QueueHandle_t     *p_xDataTransmitQueue = p_ctxNetworkActiveState->p_xDataTransmitQueue;
  char *p_cConnectedClientsCount = p_ctxNetworkActiveState->p_cConnectedClientsCount;

  // Enter High Power WiFi Mode
  wifi_disable_modem_sleep();
  vTaskDelay(pdMS_TO_TICKS(1000));

  // // Publish to Active Sensors Channel
  // esp_mqtt_client_publish(*p_mqtt_client, "active_sensors", SENSOR_ID, 0, 0, 0);

  // // Subscribe to Client Connections Channel
  // esp_mqtt_client_subscribe(*p_mqtt_client, "client_connections/"SENSOR_ID, 1);

  while(*p_eNetworkState==NETWORK_ACTIVE)
  {
    // Wait for Data to be available on the Queue
    xDataTransmitResult = xQueueReceive(*p_xDataTransmitQueue, &p_cCore0TransmitDataBufferAddress, pdMS_TO_TICKS(1000));
    if( xDataTransmitResult == pdPASS)
    {
      transmit_data_packet_to_mothership(p_mqtt_client, p_cCore0TransmitDataBufferAddress);
    }
    // Check for Network Inactive Semaphore
    xNetworkInactiveResult = xSemaphoreTake(*p_xNetworkInactiveSemaphore, 0);
    if (xNetworkInactiveResult == pdPASS)
    {
      *p_eNetworkState = NETWORK_INACTIVE;
    }
  }
}

static void transmit_data_packet_to_mothership(esp_mqtt_client_handle_t *p_mqtt_client, char *p_cCore0TransmitDataBufferAddress)
{
  static const char *TAG = "Transmit Log";
  // Create Core 1 Transmit Data Buffer
  static char seed = 1;
  static char cCore1TransmitDataBuffer[(WATERMARK_THRESHOLD*7)+1];

  // Set last value in Core1 Transmit Buffer to Null Terminator
  if (seed)
  {
    cCore1TransmitDataBuffer[WATERMARK_THRESHOLD*7] = '\0';
    seed = 0;
  }
  // Copy Data from Core0 Memory to Core1 Memory
  memcpy(cCore1TransmitDataBuffer, p_cCore0TransmitDataBufferAddress, MEMCPY_LENGTH);

  // Transmit Data on MQTT Channel
  esp_mqtt_client_publish(*p_mqtt_client, "data/"SENSOR_ID, cCore1TransmitDataBuffer, TRANSMIT_BUFFER_LENGTH, 0, 0);
}
