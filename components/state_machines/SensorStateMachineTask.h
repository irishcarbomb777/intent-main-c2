#pragma once
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// D

// Declare State Enum
typedef enum {
  SENSOR_SLEEP   = 0,
  SENSOR_ACTIVE  = 1,
  SENSOR_READY   = 2,
  SENSOR_RUNNING = 3,
} SensorStateEnum_t;

// Declare Sensor State Machine Task Context Type
typedef struct SensorStateMachineTaskContext_t {
  // Communication Protocols
  spi_device_handle_t *p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xConnectedClientsSemaphore;
  SemaphoreHandle_t *p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xStartSetSemaphore;
  SemaphoreHandle_t *p_xDataTransmitSemaphore;
  SemaphoreHandle_t *p_xEndSetSemaphore;
  SemaphoreHandle_t *p_xNetworkActiveSemaphore;
  SemaphoreHandle_t *p_xNetworkInactiveSemaphore;

  // Shared Data Buffers
  char *p_cConnectedClientsCount;
  char *p_cDataTransmitBuffer;
} SensorStateMachineTaskContext_t;

typedef struct SleepStateContext_t {
  // Sensor State
  SensorStateEnum_t *p_eSensorState;

  // Communication Protocols
  spi_device_handle_t *p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xNetworkInactiveSemaphore;
} SleepStateContext_t;

typedef struct ActiveStateContext_t {
  // Sensor State
  SensorStateEnum_t *p_eSensorState;

  // Communication Protocols
  spi_device_handle_t *p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xConnectedClientsSemaphore;
  SemaphoreHandle_t *p_xNetworkActiveSemaphore;

  // Shared Data Buffers
  char *p_cConnectedClientsCount;

} ActiveStateContext_t;

typedef struct ReadyStateContext_t {
  // Sensor State
  SensorStateEnum_t *p_eSensorState;

  // Communication Protocols
  spi_device_handle_t *p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xConnectedClientsSemaphore;

  // Shared Data Buffers
  char *p_cConnectedClientsCount;
} ReadyStateContext_t;

typedef struct RunningStateContext_t {
  // Sensor State
  SensorStateEnum_t *p_eSensorState;

  // Communication Protocols
  spi_device_handle_t *p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xStartSetSemaphore;
  SemaphoreHandle_t *p_xDataTransmitSemaphore;
  SemaphoreHandle_t *p_xEndSetSemaphore;
  SemaphoreHandle_t *p_xDataReadySemaphore;

  // Shared Data Buffers
  char *p_cDataTransmitBuffer;
} RunningStateContext_t;

// Sensor State Machine Task Prototype
void SensorStateMachineTask(void *arg);

// Utility Functions
void lsm6dsr_sleep_active_state(spi_device_handle_t *p_spi);
void read_write_magnitude_to_buffer(spi_device_handle_t *p_spi, double *p_dBuffer, unsigned int *p_uintIndex);
bool is_power_of_two(uint buffer_length)
uint circular_add(int x, uint val);
uint circular_subtract(int x, uint val)