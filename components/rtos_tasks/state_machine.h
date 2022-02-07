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
  SLEEP = 0,
  ACTIVE = 1,
  READY = 2,
  RUNNING = 3,
} sensor_state_enum_t;

typedef struct sleep_state_resources_t {
  // Communication Protocols
  spi_device_handle_t *p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xNetworkingInactiveSemaphore;
} sleep_state_resources_t;

typedef struct active_state_resources_t {
  // Communication Protocols
  spi_device_handle_t *p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xConnectedClientsSemaphore;
  SemaphoreHandle_t *p_xNetworkingActiveSemaphore;

  // Shared Data Buffers
  char *p_connected_clients_count;
} active_state_resources_t;

// Function Prototypes
void sleep_state(sleep_state_resources_t *p_sleep_state_resources, sensor_state_enum_t *p_sensor_state);
void active_state(active_state_resources_t *p_active_state_resources, sensor_state_enum_t *p_sensor_state);
