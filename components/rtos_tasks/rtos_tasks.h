#pragma once
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// Data Structures
typedef struct state_machine_task_resources_t {
  // Communication Protocols
  spi_device_handle_t *p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xNetworkInactiveSemaphore;
  SemaphoreHandle_t *p_xConnectedClientsSemaphore;
  SemaphoreHandle_t *p_xNetworkActiveSemaphore;
  SemaphoreHandle_t *p_xDataTransmitSemaphore;
  SemaphoreHandle_t *p_xStartSetSemaphore;
  SemaphoreHandle_t *p_xEndSetSemaphore;

  // Shared Data Buffers
  char *p_connected_clients_count;
  char *p_data_transmit_buffer;
} state_machine_task_resources_t;

typedef struct network_task_resources_t {
  // State Transition Semaphores
  SemaphoreHandle_t *p_xNetworkActiveSemaphore;
  SemaphoreHandle_t *p_xNetworkInactiveSemaphore;

  // Signal Semaphores
  SemaphoreHandle_t *p_xConnectedClientsSemaphore;
  SemaphoreHandle_t *p_xStartSetSemaphore;
  SemaphoreHandle_t *p_xDataTransmitSemaphore;
  SemaphoreHandle_t *p_xEndSetSemaphore;

  // Shared Data Buffers
  char *p_connected_clients_count;
  char *p_data_transmit_buffer;
} network_task_resources_t;

typedef struct rtos_tasks_shared_resources_t {
  // Task Handles
  TaskHandle_t *p_xStateMachineTaskHandle;
  TaskHandle_t *p_xNetworkTaskHandle;

  // Signal Semaphores
  SemaphoreHandle_t *p_xConnectedClientsSemaphore;
  SemaphoreHandle_t *p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xStartSetSemaphore;
  SemaphoreHandle_t *p_xDataTransmitSemaphore;
  SemaphoreHandle_t *p_xEndSetSemaphore;
  SemaphoreHandle_t *p_xNetworkActiveSemaphore;
  SemaphoreHandle_t *p_xNetworkInactiveSemaphore;

  // Shared Data Buffers
  char *p_connected_clients_count;
  char *p_data_transmit_buffer;
  
} rtos_tasks_shared_resources_t;

// Function Prototypes
rtos_tasks_shared_resources_t * rtos_tasks_startup_routine(spi_device_handle_t *p_spi);
