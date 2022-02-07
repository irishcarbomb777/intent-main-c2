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
  SemaphoreHandle_t *p_xNetworkingInactiveSemaphore;
  SemaphoreHandle_t *p_xConnectedClientsSemaphore;
  SemaphoreHandle_t *p_xNetworkingActiveSemaphore;

  // Shared Data Buffers
  char *p_connected_clients_count;
}


typedef struct sleep_task_resources_t {
  // Communication Protocols
  spi_device_handle_t *p_spi;

  // State Transition Semaphores
  SemaphoreHandle_t *p_xSleepSemaphore;
  SemaphoreHandle_t *p_xActiveSemaphore;
  SemaphoreHandle_t *p_xNetworkingInactiveSemaphore;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore;
} sleep_task_resources_t;

typedef struct active_task_resources_t {
  // Communication Protocols
  spi_device_handle_t *p_spi;

  // State Transition Semaphores
  SemaphoreHandle_t *p_xActiveSemaphore;
  SemaphoreHandle_t *p_xSleepSemaphore;
  SemaphoreHandle_t *p_xReadySemaphore;
  SemaphoreHandle_t *p_xNetworkingActiveSemaphore;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xConnectedClientsSemaphore;

  // Shared Data Buffers
  char *p_connected_clients_count;

} active_task_resources_t; 

typedef struct ready_task_resources_t {
  // Communication Protocols
  spi_device_handle_t *p_spi;

  // State Transition Semaphores
  SemaphoreHandle_t *p_xReadySemaphore;
  SemaphoreHandle_t *p_xActiveSemaphore;
  SemaphoreHandle_t *p_xRunningSemaphore;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xConnectedClientsSemaphore;

  // Shared Data Buffers
  char *p_connected_clients_count;

} ready_task_resources_t;

typedef struct running_task_resources_t {
  // Communication Protocols
  spi_device_handle_t *p_spi;

  // State Transition Semaphores
  SemaphoreHandle_t *p_xRunningSemaphore;
  SemaphoreHandle_t *p_xReadySemaphore;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xStartSetSemaphore;
  SemaphoreHandle_t *p_xDataTransmitSemaphore;
  SemaphoreHandle_t *p_xEndSetSemaphore;

  // Shared Data Buffers
  char *p_data_transmit_buffer;

} running_task_resources_t;

typedef struct networking_active_task_resources_t {
  // State Transition Semaphores
  SemaphoreHandle_t *p_xNetworkingActiveSemaphore;
  SemaphoreHandle_t *p_xNetworkingInactiveSemaphore;

  // Signal Semaphores
  SemaphoreHandle_t *p_xConnectedClientsSemaphore;
  SemaphoreHandle_t *p_xStartSetSemaphore;
  SemaphoreHandle_t *p_xDataTransmitSemaphore;
  SemaphoreHandle_t *p_xEndSetSemaphore;

  // Shared Data Buffers
  char *p_connected_clients_count;
  char *p_data_transmit_buffer;

} networking_active_task_resources_t;

typedef struct networking_inactive_task_resources_t {
  // State Transition Semaphores
  SemaphoreHandle_t *p_xNetworkingInactiveSemaphore;
  SemaphoreHandle_t *p_xNetworkingActiveSemaphore;

} networking_inactive_task_resources_t;


typedef struct rtos_tasks_shared_resources_t {
  // Task Handles
  TaskHandle_t *p_xSleepTaskHandle;
  TaskHandle_t *p_xActiveTaskHandle;
  TaskHandle_t *p_xReadyTaskHandle;
  TaskHandle_t *p_xRunningTaskHandle;
  TaskHandle_t *p_xNetworkingActiveTaskHandle;
  TaskHandle_t *p_xNetworkingInactiveTaskHandle;

  // State Transition Semaphores
  SemaphoreHandle_t *p_xSleepSemaphore;
  SemaphoreHandle_t *p_xActiveSemaphore;
  SemaphoreHandle_t *p_xReadySemaphore;
  SemaphoreHandle_t *p_xRunningSemaphore;
  SemaphoreHandle_t *p_xNetworkingActiveSemaphore;
  SemaphoreHandle_t *p_xNetworkingInactiveSemaphore;

  // Signal Semaphores
  SemaphoreHandle_t *p_xConnectedClientsSemaphore;
  SemaphoreHandle_t *p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xStartSetSemaphore;
  SemaphoreHandle_t *p_xDataTransmitSemaphore;
  SemaphoreHandle_t *p_xEndSetSemaphore;

  // Shared Data Buffers
  char *p_connected_clients_count;
  char *p_data_transmit_buffer;
  
} rtos_tasks_shared_resources_t;

// Function Prototypes
rtos_tasks_shared_resources_t * rtos_tasks_startup_routine(spi_device_handle_t *p_spi);

void activity_task(void *arg);
void inactivity_task(void *arg);
void data_transmit_task(void *arg);
