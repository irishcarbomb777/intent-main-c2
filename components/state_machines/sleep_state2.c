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

#include "SensorStateMachineTask.h"
#include "lsm6dsr.h"

void vSleepState2(SleepState2Context_t *p_ctxSleepState2)
{
  // Create Logging Tag
  static const char *TAG = "Sleep State2 Log";

  // Create Data Buffers
  static BaseType_t xConnectedClientsResult;

  // Parse Out Context Pointers
  SensorStateEnum_t *p_eSensorState               = p_ctxSleepState2->p_eSensorState;
  SensorStateEnum_t *p_ePreviousSensorState       = p_ctxSleepState2->p_ePreviousSensorState;
  spi_device_handle_t *p_spi                      = p_ctxSleepState2->p_spi;
  SemaphoreHandle_t *p_xConnectedClientsSemaphore = p_ctxSleepState2->p_xConnectedClientsSemaphore;
  SemaphoreHandle_t *p_xNetworkInactiveSemaphore  = p_ctxSleepState2->p_xNetworkInactiveSemaphore;
  char *p_cConnectedClientsCount                  = p_ctxSleepState2->p_cConnectedClientsCount;

  // Set lsm6dsr to Sleep/Active State Mode if Coming from Ready State
  if (*p_ePreviousSensorState == SENSOR_READY)
    lsm6dsr_sleep_state(p_spi);
    xSemaphoreGive(*p_xNetworkInactiveSemaphore);

  // Begin Active State Loop
  while (*p_eSensorState == SENSOR_SLEEP)
  {
    // Wait for Client to Connect
    xConnectedClientsResult = xSemaphoreTake(*p_xConnectedClientsSemaphore, pdMS_TO_TICKS(1000));
    if (xConnectedClientsResult)
      if ((*p_cConnectedClientsCount) > 0)
      {
        // Set Next State
        *p_ePreviousSensorState = *p_eSensorState;
        *p_eSensorState = SENSOR_READY;
      }
  }
}

// Declare Static Functions
