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

// Define Sleep State Constant Settings
#define TapWindow_Length 16
#define TapDetectionKernelLength 2
#define Tap_MinDelta 1000
#define Tap_Target 2


void vSleepState(SleepStateContext_t *p_ctxSleepState)
{
  // Create Logging Tag
  static const char *TAG = "Sleep State Log";

  // Create Data Buffers
  static double dTapWindow[TapWindow_Length];
  static uint uintTapWindowIndex = 0;
  static BaseType_t xDataReadyResult;

  // Parse out Context Pointers
  SensorStateEnum_t   *p_eSensorState              = p_ctxSleepState->p_eSensorState;
  spi_device_handle_t *p_spi                       = p_ctxSleepState->p_spi;
  SemaphoreHandle_t   *p_xDataReadySemaphore       = p_ctxSleepState->p_xDataReadySemaphore;
  SemaphoreHandle_t   *p_xNetworkInactiveSemaphore = p_ctxSleepState->p_xNetworkInactiveSemaphore;

  // Give Network Inactive Semaphore
  xSemaphoreGive(*p_xNetworkInactiveSemaphore)

  // Begin Sleep State Loop
  while (*p_eSensorState == SENSOR_SLEEP)
  {
    // Wait for Data to be Ready
    xDataReadyResult = xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(1000));
    if (xDataReadyResult)
    {
      // Read Data
      read_write_magnitude_to_buffer(p_spi, &dTapWindow, &uintTapWindowIndex);

      // Perform Tap Detection
      if (tap_detection(&dTapWindow, &uintTapWindowIndex))
      {
        // Reset Data Buffers inside sleep state
        memset(&dTapWindow, 0, sizeof(dTapWindow));
        uintTapWindowIndex = 0;

        // Set Next State
        *p_eSensorState = SENSOR_ACTIVE;
      }
    }
    else
      ESP_LOGI(TAG, "Data Ready Interrupt Timed Out... Clearing Data Register");
      char data_clear_buffer[7];
      lsm6dsr_read_single_xl_measurement(p_spi, data_clear_buffer);
  }
}

static char tap_detection( double *p_dTapWindow, unsigned int *p_uintTapWindowIndex)
{
  // Create return Variable
  static char tap_condition_detected = 0;

  // Create Static Data Buffers
  static char taps_detected_history_buffer[TapWindow_Length];
  static double tap_delta = 0;
  static char taps_detected = 0;

  // Calculate Tap Delta
  for (int k=0; k < TapDetectionKernelLength; k++)
      tap_delta +=   *(p_dTapWindow + circular_subtract(k, *p_uintTapWindowIndex, TapWindow_Length)) 
                   - *(p_dTapWindow + circular_subtract((k+1), *p_uintTapWindowIndex, TapWindow_Length));

  // Check if Tap Delta Exceeds Threshold
  if (tap_delta > Tap_MinDelta)
  {
    taps_detected += 1;
    taps_detected_history_buffer[*p_uintTapWindowIndex] = 1;
  }
  else
  {
    taps_detected_history_buffer[*p_uintTapWindowIndex] = 0;
  }
  tap_delta = 0;  // Reset Tap Delta

  // Increment Tap Window Index and Subtract Old Detection Value
  *p_uintTapWindowIndex = circular_add(1, *p_uintTapWindowIndex, TapWindow_Length);
  taps_detected -= (taps_detected_history_buffer[*p_uintTapWindowIndex] == 1) ? 0 : 1;

  // If taps detected is at target, reset buffers and return true
  if (taps_detected > (Tap_Target-1)
  {
    tap_condition_detected = 0;
    tap_delta = 0;
    taps_detected = 0;
    memset(&taps_detected_history_buffer, 0, sizeof(taps_detected_history_buffer));
    return 1;
  }
  else
    return 0;
}

