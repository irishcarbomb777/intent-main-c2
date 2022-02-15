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
  SemaphoreHandle_t *p_xDataTransmitQueue;
  SemaphoreHandle_t *p_xEndSetSemaphore;
  SemaphoreHandle_t *p_xNetworkActiveSemaphore;
  SemaphoreHandle_t *p_xNetworkInactiveSemaphore;

  // Shared Data Buffers
  char *p_cConnectedClientsCount;
} SensorStateMachineTaskContext_t;

typedef struct SleepStateContext_t {
  // Sensor State
  SensorStateEnum_t *p_eSensorState;
  SensorStateEnum_t *p_ePreviousSensorState;

  // Communication Protocols
  spi_device_handle_t *p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xNetworkInactiveSemaphore;
} SleepStateContext_t;

typedef struct SleepState2Context_t {
  // Sensor State
  SensorStateEnum_t *p_eSensorState;
  SensorStateEnum_t *p_ePreviousSensorState;

  // Communication Protocols
  spi_device_handle_t *p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xConnectedClientsSemaphore;
  SemaphoreHandle_t *p_xNetworkInactiveSemaphore;

  // Shared Data Buffers
  char *p_cConnectedClientsCount;

} SleepState2Context_t;

typedef struct ActiveStateContext_t {
  // Sensor State
  SensorStateEnum_t *p_eSensorState;
  SensorStateEnum_t *p_ePreviousSensorState;

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
  SensorStateEnum_t *p_ePreviousSensorState;

  // Communication Protocols
  spi_device_handle_t *p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xConnectedClientsSemaphore;
  QueueHandle_t *p_xDataTransmitQueue;
  SemaphoreHandle_t *p_xNetworkActiveSemaphore;
  // Shared Data Buffers
  char *p_cConnectedClientsCount;
} ReadyStateContext_t;

typedef struct RunningStateContext_t {
  // Sensor State
  SensorStateEnum_t *p_eSensorState;
  SensorStateEnum_t *p_ePreviousSensorState;

  // Communication Protocols
  spi_device_handle_t *p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xStartSetSemaphore;
  QueueHandle_t *p_xDataTransmitQueue;
  SemaphoreHandle_t *p_xEndSetSemaphore;
  SemaphoreHandle_t *p_xDataReadySemaphore;
  
} RunningStateContext_t;

// Sensor State Machine Task Prototype
void SensorStateMachineTask(void *arg);

// Sensor State Functions
void vSleepState(SleepStateContext_t *p_ctxSleepState);
void vSleepState2(SleepState2Context_t *p_ctxSleepState2);
void vActiveState(ActiveStateContext_t *p_ctxActiveState);
void vReadyState(ReadyStateContext_t *p_ctxReadyState);
// void vRunningState(RunningStateContext_t *p_ctxRunningState);

// Utility Functions
void lsm6dsr_sleep_state(spi_device_handle_t *p_spi);
void lsm6dsr_sleep_active_state(spi_device_handle_t *p_spi);
void read_write_magnitude_to_buffer(spi_device_handle_t *p_spi, double *p_dBuffer, uint *p_uintIndex);
uint circular_add(int x, uint val, uint buffer_length);
uint circular_subtract(int x, uint val, uint buffer_length);