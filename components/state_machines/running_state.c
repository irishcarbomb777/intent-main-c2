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

// Define Ready State Constant Settings
#define WATERMARK_THRESHOLD 300
#define FifoReadDataBufferLength ((WATERMARK_THRESHOLD*7)+1)
#define TxPacketSize (WATERMARK_THRESHOLD*7)
#define TxBufferLength (TxPacketSize*5)
// Delcare Static Function Prototypes
static char detect_inactivity(char *p_cFifoBuffer);

void vRunningState(RunningStateContext_t *p_ctxRunningState)
{
  // Create Logging Tag
  static const char *TAG = "Running State Log";

  // Create Data Buffers
  static BaseType_t xDataReadyResult;
  static BaseType_t xDataTransmitResult;
  static char cFifoReadDataBuffer[FifoReadDataBufferLength];
  static char cTxBuffer[TxBufferLength];
  static uint uintTxBufferIndex = 0;
  static char *p_cPointToTxBufferAddress;

  // Parse Out Context Pointers
  spi_device_handle_t *p_spi                = p_ctxRunningState->p_spi;
  SensorStateEnum_t *p_eSensorState         = p_ctxRunningState->p_eSensorState;
  SensorStateEnum_t *p_eSensorPreviousState = p_ctxRunningState->p_ePreviousSensorState;
  SemaphoreHandle_t *p_xEndSetSemaphore     = p_ctxRunningState->p_xEndSetSemaphore;
  SemaphoreHandle_t *p_xDataReadySemaphore  = p_ctxRunningState->p_xDataReadySemaphore;
  QueueHandle_t *p_xDataTransmitQueue       = p_ctxRunningState->p_xDataTransmitQueue;

  // Begin Running State Loop
  while (*p_eSensorState == SENSOR_RUNNING)
  {
    xDataReadyResult = xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(1000));
    if ( xDataReadyResult == pdPASS )
    {
      // Read Data from FIFO
      lsm6dsr_batch_read_fifo(p_spi, WATERMARK_THRESHOLD, cFifoReadDataBuffer);

      // Copy Data to Tx Buffer
      circular_memcpy((cFifoReadDataBuffer+1), (FifoReadDataBufferLength-1), cTxBuffer, &uintTxBufferIndex, TxBufferLength);

      // Send Pointer of Current Location in Tx Buffer to Transmit Queue
      p_cPointToTxBufferAddress = &cTxBuffer[uintTxBufferIndex];
      xDataTransmitResult = xQueueSend(*p_xDataTransmitQueue, (void*)(&p_cPointToTxBufferAddress), 0);
      if ( detect_inactivity((cFifoReadDataBuffer+1)) )
      {
        ESP_LOGI(TAG, "Inactivity Detected!");
        
        // Perform State Transition
        *p_eSensorPreviousState = *p_eSensorState;
        *p_eSensorState = SENSOR_READY;
      }
       
    }
    else
    {
      ESP_LOGI(TAG, "Data Ready Timeout!");
      // Clear Data from FIFO
      lsm6dsr_clear_FIFO(p_spi);
    }
  }
}

static char detect_inactivity(char *p_cFifoBuffer)
{
  // Detect Inactivity Settings
  #define INACTIVITY_SECONDS_THRESHOLD 5
  #define SAMPLES_PER_SECOND 66 // Equivalent to 3 Samples per FIFO Read
  #define INACTIVITY_DETECTION_BUFFER_LENGTH 10
  #define MS_PER_SAMPLE 15
  #define DATAPOINT0_PRESEED_MAGNITUDE 8234
  #define FIFO_JUMP_SIZE (100)
  #define PERCENT_FROM_1G (0.12)
  #define INACTIVITY_DERIVATIVE_SUM_THRESHOLD (DATAPOINT0_PRESEED_MAGNITUDE - (DATAPOINT0_PRESEED_MAGNITUDE*PERCENT_FROM_1G))
  // Create Tag
  static const char *TAG = "Inactivity Detection Log";

  // Declare Static Data Buffers
  static int16_t xyz_data_buffer[3];
  static double dInactivityDetectionBuffer[INACTIVITY_DETECTION_BUFFER_LENGTH] = {DATAPOINT0_PRESEED_MAGNITUDE};
  static uint uintInactivityDetectionBufferIndex = 1;
  static uint uintInactivityCount_ms = 0;
  static uint uintActivityCount_ms = 0;
  static double dRunningSumOfAbsDifference = 0;
  static double dAbsoluteDerivative = 0;
  
  for (uint i=0; i < (FifoReadDataBufferLength-1); i += 7*FIFO_JUMP_SIZE)
  {
    // Parse Data from Bytes to an xyz uint16 buffer
    lsm6dsr_parse_read_data_buffer((p_cFifoBuffer+i), xyz_data_buffer);

    // Calculate magnitude from xyz buffer and place into Target Buffer
    *(dInactivityDetectionBuffer+uintInactivityDetectionBufferIndex) = (pow( (pow( (double) xyz_data_buffer[0], 2) + pow((double) xyz_data_buffer[1], 2) + pow((double) xyz_data_buffer[2], 2)), 0.5 ));
    // ESP_LOGI(TAG, "Magnitude: %.2f", *(dInactivityDetectionBuffer+uintInactivityDetectionBufferIndex));

    // Calculate Most Recent Absolute Derivative (mag[n]-mag[n-1])
    dAbsoluteDerivative = abs(   *(dInactivityDetectionBuffer+uintInactivityDetectionBufferIndex)
                               - *(dInactivityDetectionBuffer + circular_subtract(1, uintInactivityDetectionBufferIndex, INACTIVITY_DETECTION_BUFFER_LENGTH)));
    if (dAbsoluteDerivative < INACTIVITY_DERIVATIVE_SUM_THRESHOLD )
    {
      uintInactivityCount_ms += MS_PER_SAMPLE;
      if (uintInactivityCount_ms  > 1000*INACTIVITY_SECONDS_THRESHOLD)
      {
        // Reset Static Variables
        uintInactivityDetectionBufferIndex = 1;
        memset(dInactivityDetectionBuffer, 0, INACTIVITY_DETECTION_BUFFER_LENGTH*sizeof(double));
        dInactivityDetectionBuffer[0] = DATAPOINT0_PRESEED_MAGNITUDE;
        uintInactivityCount_ms = 0;
        uintActivityCount_ms = 0;
        // Set Inactivity Detected
        return 1;
      }
    }
    else
    {
      uintActivityCount_ms += MS_PER_SAMPLE;
      if (uintActivityCount_ms > 250)
      {
        uintActivityCount_ms = 0;
        uintInactivityCount_ms = 0;
      }
    }
    // Increment the index of the detection buffer for next Loop
    uintInactivityDetectionBufferIndex = circular_add(1, uintInactivityDetectionBufferIndex, INACTIVITY_DETECTION_BUFFER_LENGTH);
  }
  return 0;
}

// OTHER ALGORITHM OPTION!!!
// // Calculate Most Recent Absolute Derivative (mag[n]-mag[n-1])
//     dRunningSumOfAbsDifference += abs(   *(dInactivityDetectionBuffer+uintInactivityDetectionBufferIndex)
//                                        - *(dInactivityDetectionBuffer + circular_subtract(1, uintInactivityDetectionBufferIndex, INACTIVITY_DETECTION_BUFFER_LENGTH)));
    
//     // Subtract Least Recent Absolute Derivative (mag[n+2]-mag[n+1])
//     dRunningSumOfAbsDifference -= abs(  *(dInactivityDetectionBuffer + circular_add(2, uintInactivityDetectionBufferIndex, INACTIVITY_DETECTION_BUFFER_LENGTH))
//                                       - *(dInactivityDetectionBuffer + circular_add(1, uintInactivityDetectionBufferIndex, INACTIVITY_DETECTION_BUFFER_LENGTH)));
//     ESP_LOGI(TAG, "dRunningSumofAbs Diff: %.2f", dRunningSumOfAbsDifference);
//     // Check if Running Sum of Absolute Difference Is Less Than Threshold
//     if (dRunningSumOfAbsDifference < INACTIVITY_DERIVATIVE_SUM_THRESHOLD)
//     {
//       uintInactivityCount_ms += MS_PER_SAMPLE;
//       // Reset Activity ms Count if Inactivity Window Occurs
//       uintActivityCount_ms = 0;
//       if (uintInactivityCount_ms > 1000*INACTIVITY_SECONDS_THRESHOLD)
//       {
//         // Reset Static Variables
//         dRunningSumOfAbsDifference = 0;
//         uintInactivityDetectionBufferIndex = 1;
//         memset(dInactivityDetectionBuffer, 0, INACTIVITY_DETECTION_BUFFER_LENGTH*sizeof(double));
//         dInactivityDetectionBuffer[0] = DATAPOINT0_PRESEED_MAGNITUDE;
//         uintInactivityCount_ms = 0;
//         // Set Inactivity Detected
//         return 1;
//       }
//     }
//     else  // If Activity happens for more than 0.5 seconds, reset Inactivity Count
//     {
//       uintActivityCount_ms += MS_PER_SAMPLE;
//       if (uintActivityCount_ms > 500)
//       {
//         // Reset Inactivity Count
//         dRunningSumOfAbsDifference = 0;
//         uintInactivityDetectionBufferIndex = 0;
//         memset(dInactivityDetectionBuffer, 0, INACTIVITY_DETECTION_BUFFER_LENGTH*sizeof(double));
//         dInactivityDetectionBuffer[0] = DATAPOINT0_PRESEED_MAGNITUDE;
//         uintInactivityCount_ms = 0;
//       }
//     }