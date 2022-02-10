#pragma once
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "mqtt_client.h"

// Declare State Enum
typedef enum {
  NETWORK_INACTIVE = 0,
  NETWORK_ACTIVE   = 1,
} NetworkStateEnum_t;

// Declare Network State Machine Task Context Type
typedef struct NetworkStateMachineTaskContext_t {
  // Signal Semaphores
  SemaphoreHandle_t *p_xConnectedClientsSemaphore;
  SemaphoreHandle_t *p_xStartSetSemaphore;
  SemaphoreHandle_t *p_xDataTransmitSemaphore;
  SemaphoreHandle_t *p_xEndSetSemaphore;
  SemaphoreHandle_t *p_xNetworkActiveSemaphore;
  SemaphoreHandle_t *p_xNetworkInactiveSemaphore;
  

  // Shared Data Buffers
  char *p_cConnectedClientsCount;
  char *p_cDataTransmitBuffer;
} NetworkStateMachineTaskContext_t;

typedef struct NetworkActiveStateContext_t {
  // Communication Protocols
  esp_mqtt_client_handle_t *p_mqtt_client;

  // Signal Semaphores
  SemaphoreHandle_t *p_xConnectedClientsSemaphore;
  SemaphoreHandle_t *p_xStartSetSemaphore;
  SemaphoreHandle_t *p_xDataTransmitSemaphore;
  SemaphoreHandle_t *p_xEndSetSemaphore;
  SemaphoreHandle_t *p_xNetworkInactiveSemaphore;

  // Shared Data Buffers
  char *p_connected_clients_count;
  char *p_data_transmit_buffer;
} NetworkActiveStateContext_t;

typedef struct NetworkInactiveStateContext_t {
  // Communication Protocols
  esp_mqtt_client_handle_t *p_mqtt_client;

  // Signal Semaphores
  SemaphoreHandle_t *p_xNetworkActiveSemaphore;
} NetworkInactiveStateContext_t;

// Network State Machine Task Prototype
void NetworkStateMachineTask (void *arg);