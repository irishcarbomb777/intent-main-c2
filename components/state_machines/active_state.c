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

// Define Active State Constant Settings
#define NO_CONNECT_TIMEOUT_SECONDS (10)

void vActiveState(ActiveStateContext_t *p_ctxActiveState)
{
  // Create Logging Tag
  static const char *TAG = "Active State Log";

  // Create Data Buffers
  static BaseType_t xConnectedClientsResult;
  static uint uintNoConnectionSecondsCount = 0;

  // Parse Out Context Pointers
  SensorStateEnum_t *p_eSensorState               = p_ctxActiveState->p_eSensorState;
  spi_device_handle_t *p_spi                      = p_ctxActiveState->p_spi;
  SemaphoreHandle_t *p_xConnectedClientsSemaphore = p_ctxActiveState->p_xConnectedClientsSemaphore;
  SemaphoreHandle_t *p_xNetworkActiveSemaphore    = p_ctxActiveState->p_xNetworkActiveSemaphore;
  char *p_cConnectedClientsCount                  = p_ctxActiveState->p_cConnectedClientsCount;

  // Give Network Active Semaphore
  xSemaphoreGive(*p_xNetworkActiveSemaphore);

  // Set lsm6dsr to Sleep/Active State Mode
  lsm6dsr_sleep_active_state(p_spi);

  // Begin Active State Loop
  while (*p_eSensorState == SENSOR_ACTIVE)
  {
    // Wait for Client to Connect
    xConnectedClientsResult = xSemaphoreTake(*p_xConnectedClientsSemaphore, pdMS_TO_TICKS(1000));
    if (xConnectedClientsResult)
      if ((*p_cConnectedClientsCount) > 0)
      {
        // Reset Data Buffers inside Active State
        uintNoConnectionSecondsCount = 0;
        // Set Next State
        *p_eSensorState = SENSOR_READY;
      }
      else
        uintNoConnectionSecondsCount++;
    else
      uintNoConnectionSecondsCount++;
    
    // Check if Active State Timeout has Occured
    if (uintNoConnectionSecondsCount > NO_CONNECT_TIMEOUT_SECONDS)
    {
      // Reset Data Buffers inside Active State
      uintNoConnectionSecondsCount = 0;
      *p_eSensorState = SENSOR_SLEEP;
    }
  }
}