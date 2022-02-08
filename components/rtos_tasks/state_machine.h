#pragma once
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"


// Declare Data Structures
typedef enum {
  SENSOR_SLEEP   = 0,
  SENSOR_ACTIVE  = 1,
  SENSOR_READY   = 2,
  SENSOR_RUNNING = 3,
} sensor_state_enum_t;

typedef enum {
  NETWORK_INACTIVE = 0,
  NETWORK_ACTIVE   = 1,
} network_state_enum_t;

typedef struct network_active_state_resources_t {
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
} network_active_state_resources_t;

typedef struct network_inactive_state_resources_t {
  // Communication Protocols
  esp_mqtt_client_handle_t *p_mqtt_client;

  // Signal Semaphores
  SemaphoreHandle_t *p_xNetworkActiveSemaphore;
} network_inactive_state_resources_t;

typedef struct sleep_state_resources_t {
  // Communication Protocols
  spi_device_handle_t *p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xNetworkInactiveSemaphore;
} sleep_state_resources_t;

typedef struct active_state_resources_t {
  // Communication Protocols
  spi_device_handle_t *p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xConnectedClientsSemaphore;
  SemaphoreHandle_t *p_xNetworkActiveSemaphore;

  // Shared Data Buffers
  char *p_connected_clients_count;
} active_state_resources_t;

typedef struct ready_state_resources_t {
  // Communication Protocols
  spi_device_handle_t *p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xConnectedClientsSemaphore;

  // Shared Data Buffers
  char *p_connected_clients_count;
} ready_state_resources_t;

typedef struct running_state_resources_t {
  // Communication Protocols
  spi_device_handle_t *p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xStartSetSemaphore;
  SemaphoreHandle_t *p_xDataTransmitSemaphore;
  SemaphoreHandle_t *p_xEndSetSemaphore;
  SemaphoreHandle_t *p_xDataReadySemaphore;

  // Shared Data Buffers
  char *p_data_transmit_buffer;
} running_state_resources_t;

// Function Prototypes
void sleep_state(sleep_state_resources_t *p_sleep_state_resources, sensor_state_enum_t *p_sensor_state);
void active_state(active_state_resources_t *p_active_state_resources, sensor_state_enum_t *p_sensor_state);
void ready_state(ready_state_resources_t *p_ready_state_resources, sensor_state_enum_t *p_sensor_state);
void running_state(running_state_resources_t *p_running_state_resources, sensor_state_enum_t *p_sensor_state);

void network_active_state(network_active_state_resources_t *p_network_active_state_resources, network_state_enum_t *p_network_state);
void network_inactive_state(network_inactive_state_resources_t *p_network_inactive_state_resources, network_state_enum_t *p_network_state);

