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
#define TapWindow_Length 32
#define TapDetectionKernelLength 8
#define Tap_MinDelta 500
#define Tap_Target 2

// Declare Static Function Prototypes
static char tap_detection( double *p_dTapWindow, unsigned int *p_uintTapWindowIndex);
static void clear_junk_data( spi_device_handle_t *p_spi, SemaphoreHandle_t *p_xDataReadySemaphore);
static void clear_tap_kernel(double *p_dTapWindow);

void vSleepState(SleepStateContext_t *p_ctxSleepState)
{
  // Create Logging Tag
  static const char *TAG = "Sleep State Log";

  // Create Static Data Buffers
  static double dTapWindow[TapWindow_Length];
  static uint uintTapWindowIndex = 0;
  static BaseType_t xDataReadyResult;

  // Set Static Data Buffers
  clear_tap_kernel(dTapWindow);

  // Parse out Context Pointers
  SensorStateEnum_t   *p_eSensorState              = p_ctxSleepState->p_eSensorState;
  SensorStateEnum_t   *p_ePreviousSensorState      = p_ctxSleepState->p_ePreviousSensorState;
  spi_device_handle_t *p_spi                       = p_ctxSleepState->p_spi;
  SemaphoreHandle_t   *p_xDataReadySemaphore       = p_ctxSleepState->p_xDataReadySemaphore;
  SemaphoreHandle_t   *p_xNetworkInactiveSemaphore = p_ctxSleepState->p_xNetworkInactiveSemaphore;

  // Give Network Inactive Semaphore
  xSemaphoreGive(*p_xNetworkInactiveSemaphore);

  // Clear Junk Data
  clear_junk_data(p_spi, p_xDataReadySemaphore);

  // Begin Sleep State Loop
  while (*p_eSensorState == SENSOR_SLEEP)
  {
    // Wait for Data to be Ready
    xDataReadyResult = xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(1000));
    if (xDataReadyResult)
    {
      // Read Data
      read_write_magnitude_to_buffer(p_spi, dTapWindow, &uintTapWindowIndex);

      // Perform Tap Detection
      if (tap_detection(dTapWindow, &uintTapWindowIndex))
      {
        // Set Next State
        *p_ePreviousSensorState = *p_eSensorState;
        *p_eSensorState = SENSOR_ACTIVE;
      }
    }
    else
    {
      ESP_LOGI(TAG, "Data Ready Interrupt Timed Out... Clearing Data Register");
      char data_clear_buffer[7];
      lsm6dsr_read_single_xl_measurement(p_spi, data_clear_buffer);
    }
  }
}

static char tap_detection( double *p_dTapWindow, unsigned int *p_uintTapWindowIndex)
{
  // Create Logging Tag
  static const char *TAG = "Tap Detection Log";

  // Create Static Data Buffers
  static char taps_detected_history_buffer[TapWindow_Length];
  static char taps_detected = 0;
  static double tap_delta = 0;

  // Create State of Tap Delta
  enum TapDetectionStateEnum{
    ACTIVE,
    SETTLING,
  };
  static enum TapDetectionStateEnum TapState = ACTIVE; 

  // Calculate Moving Average of Tap Delta
  for (int k=0; k < TapDetectionKernelLength; k++)
    tap_delta += abs(  *(p_dTapWindow + circular_subtract(k, *p_uintTapWindowIndex, TapWindow_Length)) 
                    - *(p_dTapWindow + circular_subtract((k+1), *p_uintTapWindowIndex, TapWindow_Length)));
  tap_delta = tap_delta / TapDetectionKernelLength;

  switch (TapState)
  {
    case ACTIVE:
      // Check if Tap Delta Exceeds Threshold
      if (tap_delta > Tap_MinDelta)
      {
        taps_detected += 1;
        taps_detected_history_buffer[*p_uintTapWindowIndex] = 1;

        // Set State to Settling
        TapState = SETTLING;
      }
      else
        taps_detected_history_buffer[*p_uintTapWindowIndex] = 0;
      break; 
    case SETTLING:
      if (tap_delta < Tap_MinDelta+300)
      {
        clear_tap_kernel(p_dTapWindow);
        TapState = ACTIVE;
      } 
      break;
    default:
      TapState = ACTIVE;
      break;
  }

  tap_delta = 0;  // Reset Tap Delta

  // Increment Tap Window Index and Subtract Old Detection Value
  *p_uintTapWindowIndex = circular_add(1, *p_uintTapWindowIndex, TapWindow_Length);
  if (taps_detected > 0)
    taps_detected -= ((taps_detected_history_buffer[*p_uintTapWindowIndex] == 1) ? 1 : 0);

  // If taps detected is at target, reset buffers and return true
  if (taps_detected > (Tap_Target-1))
  {
    tap_delta = 0;
    taps_detected = 0;
    clear_tap_kernel(p_dTapWindow);
    return 1;
  }  
  return 0;
}

static void clear_junk_data(spi_device_handle_t *p_spi, SemaphoreHandle_t *p_xDataReadySemaphore)
{
  // Clear out Junk Data
  static char cJunkDataBuffer[7];
  for (int i=0; i < 10; i++)
  {
    xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(1000));
    lsm6dsr_read_single_xl_measurement(p_spi, cJunkDataBuffer);
  }
}

static void clear_tap_kernel(double *p_dTapWindow)
{
  // Clear the Kernel
  for (int i=0; i < TapWindow_Length; i++)
    *(p_dTapWindow+i) = 8234;
}
