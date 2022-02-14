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
#define ReadyState_XL_ODR 6667
#define RollbackBuffer_length 18000 // 2.69 seconds
#define RollbackBuffer_byteLength (RollbackBuffer_length*7)
#define Decimation_Factor 3         // Roughly 444 samples / 0.2s
#define SetDetectionBufferLength 512  // Roughly 0.23s buffer length
#define Datapoint0_1g_Preseed 8234
#define PercentFrom1g (0.12)
#define SetDetectionDerivativeSumThreshold (Datapoint0_1g_Preseed - (Datapoint0_1g_Preseed*PercentFrom1g))

// Declare Static Function Prototypes
static void ResetReadyStateMemoryTask(void *arg);
static char detect_set(uint uintDecimationFactor, char *p_cSourceBuffer, uint uintSourceBufferLength);
static void circular_memcpy( char *p_cSourceBuffer, uint uintSourceBufferLength,
                             char *p_cTargetBuffer, uint *p_uintTargetBufferIndex, 
                             uint uintTargetBufferLength);
static void lsm6dsr_ready_running_state(spi_device_handle_t *p_spi);


void vReadyState(ReadyStateContext_t *p_ctxReadyState)
{
  // Create Logging Tag
  static const char *TAG = "Ready State Log";
  ESP_LOGI(TAG, "Entering Ready State!");
  // Create Data Buffers
  static BaseType_t xDataReadyResult;
  static BaseType_t xDataTransmitResult;
  static BaseType_t xConnectedClientsResult;
  static char cFifoReadDataBuffer[FifoReadDataBufferLength];
  static char cRollbackBuffer[RollbackBuffer_byteLength];
  static uint cRollbackBufferIndex = 0;
  ESP_LOGI(TAG, "Finished Allocation!");

  // Parse Out Context Pointers
  SensorStateEnum_t *p_eSensorState               = p_ctxReadyState->p_eSensorState;
  SensorStateEnum_t *p_ePreviousSensorState       = p_ctxReadyState->p_ePreviousSensorState;
  spi_device_handle_t *p_spi                      = p_ctxReadyState->p_spi;
  SemaphoreHandle_t *p_xDataReadySemaphore        = p_ctxReadyState->p_xDataReadySemaphore;
  QueueHandle_t *p_xDataTransmitQueue             = p_ctxReadyState->p_xDataTransmitQueue;
  SemaphoreHandle_t *p_xConnectedClientsSemaphore = p_ctxReadyState->p_xConnectedClientsSemaphore;
  char *p_cConnectedClientsCount                  = p_ctxReadyState->p_cConnectedClientsCount;

  // Set lsm6dsr to High Data Rate if Coming from Sensor Ready
  if (*p_ePreviousSensorState == SENSOR_ACTIVE)
    lsm6dsr_ready_running_state(p_spi);

  // Begin Ready State Loop
  while (*p_eSensorState == SENSOR_READY)
  {
    xDataReadyResult = xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(1000));
    if ( xDataReadyResult == pdPASS )
    {
      // Read Data from FIFO
      lsm6dsr_batch_read_fifo(p_spi, WATERMARK_THRESHOLD, cFifoReadDataBuffer);

      // Copy Data to Circular Buffer
      circular_memcpy((cFifoReadDataBuffer+1), (FifoReadDataBufferLength-1), cRollbackBuffer, &cRollbackBufferIndex, RollbackBuffer_byteLength);

      // Decimate Data and Perform Set Detection
      if (detect_set( Decimation_Factor, (cFifoReadDataBuffer+1), (FifoReadDataBufferLength-1)))
      {
        ESP_LOGI(TAG, "Detected Set");

        // Push Data from Rollback Buffer onto Network Task Queue
        for (int i=0; i < (RollbackBuffer_length/WATERMARK_THRESHOLD); i++)
        {
          
          xDataTransmitResult = xQueueSend(*p_xDataTransmitQueue, (cRollbackBuffer+cRollbackBufferIndex), 0);
          cRollbackBufferIndex = circular_add((WATERMARK_THRESHOLD*7), cRollbackBufferIndex, RollbackBuffer_byteLength);
          if (!xDataTransmitResult)
            ESP_LOGI(TAG, "Add to Queue Failed!");
        }
        // Reset Data Buffer Resources
        cRollbackBufferIndex = 0;
        xTaskCreatePinnedToCore(ResetReadyStateMemoryTask, "Reset Memory Task", 4096, (void*)cRollbackBuffer, 1, NULL, 0);

        // Perform State Transition
        *p_ePreviousSensorState = *p_eSensorState;
        *p_eSensorState = SENSOR_RUNNING;
      }
    } 
    else
    {
      ESP_LOGI(TAG, "Data Ready Timeout!");
      // Read Data from FIFO
      lsm6dsr_batch_read_fifo(p_spi, WATERMARK_THRESHOLD, cFifoReadDataBuffer);

    }
    // Check for Connected Clients
    xConnectedClientsResult = xSemaphoreTake(*p_xConnectedClientsSemaphore, 0);
    if (xConnectedClientsResult && (*p_cConnectedClientsCount) < 1)
    {
      // Reset Data Buffer Resources
      cRollbackBufferIndex = 0;
      memset(cRollbackBuffer, 0, RollbackBuffer_byteLength);
      // Perform State Transition
      *p_ePreviousSensorState = *p_eSensorState;
      *p_eSensorState = SENSOR_ACTIVE;
    }
  }
}

static void ResetReadyStateMemoryTask(void *arg)
{ // Task resets memory of buffer and then kills self
  
  // Create Logging Tag
  static const char *TAG = "Reset Ready State Memory Task Log";

  // Parse out pointers to be reset
  char *p_cRollbackBuffer = (char *)arg;

  // Reset Buffer
  memset(p_cRollbackBuffer, 0, RollbackBuffer_byteLength);

  vTaskDelete(NULL);
}

static char detect_set(uint uintDecimationFactor, char *p_cSourceBuffer, uint uintSourceBufferLength)
{
  // Calculate Number of Data Copies
  static uint data_copies = ((FifoReadDataBufferLength)/7)/Decimation_Factor;
  static int16_t xyz_data_buffer[3];

  // Create Variables local to function
  static double dSetDetectionBuffer[SetDetectionBufferLength] = {Datapoint0_1g_Preseed};
  static uint uintSetDetectionBufferIndex = 1;

  static const double dRunningSumOfDifferenceThreshold = SetDetectionDerivativeSumThreshold;
  static double dRunningSumOfDifference = 0;

  for (uint i=0; i < (data_copies*7*uintDecimationFactor); i+=7*uintDecimationFactor)
  {
    // Parse Data from Bytes to an xyz uint16 Buffer
    lsm6dsr_parse_read_data_buffer((p_cSourceBuffer+i), xyz_data_buffer);

    // Calculate magnitude from xyz buffer and place into Target Buffer
    *(dSetDetectionBuffer+uintSetDetectionBufferIndex) = (pow( (pow( (double) xyz_data_buffer[0], 2) + pow((double) xyz_data_buffer[1], 2) + pow((double) xyz_data_buffer[2], 2)), 0.5 ));

    // Calculate Derivative (mag[n]-mag[n-1]) and add to running sum
    dRunningSumOfDifference +=   *(dSetDetectionBuffer+uintSetDetectionBufferIndex)
                               - *(dSetDetectionBuffer + circular_subtract(1, uintSetDetectionBufferIndex, SetDetectionBufferLength));

    // Subtract Least Recent Derivative (mag[n+2]-mag[n+1])
    dRunningSumOfDifference -=   *(dSetDetectionBuffer + circular_add(2, uintSetDetectionBufferIndex, SetDetectionBufferLength))
                               - *(dSetDetectionBuffer + circular_add(1, uintSetDetectionBufferIndex, SetDetectionBufferLength));

    if (dRunningSumOfDifference > dRunningSumOfDifferenceThreshold)
    { 
      // Reset Static Variabels
      dRunningSumOfDifference = 0;
      uintSetDetectionBufferIndex = 1;
      dSetDetectionBuffer[0] = Datapoint0_1g_Preseed;
      // Set Start Detected
      return 1;
    }
    // Increment the index of the detection buffer for next FIFO read
    uintSetDetectionBufferIndex = circular_add(1, uintSetDetectionBufferIndex, SetDetectionBufferLength);
  }
  return 0;
}

static void circular_memcpy( char *p_cSourceBuffer, uint uintSourceBufferLength,
                             char *p_cTargetBuffer, uint *p_uintTargetBufferIndex, 
                             uint uintTargetBufferLength)
{
  // Check if Source Buffer fits in current loop of Target Buffer
  if ((*p_uintTargetBufferIndex+uintSourceBufferLength) < uintTargetBufferLength)
  {
    // If Source Buffer fits, memcpy
    memcpy( (p_cTargetBuffer+*p_uintTargetBufferIndex), p_cSourceBuffer, uintSourceBufferLength);
    *p_uintTargetBufferIndex += uintSourceBufferLength;
  }
  else
  {
    uint target_space_remaining = uintTargetBufferLength - *p_uintTargetBufferIndex;
    memcpy ( (p_cTargetBuffer+*p_uintTargetBufferIndex), p_cSourceBuffer, target_space_remaining);
    uint source_bytes_remaining = uintSourceBufferLength - target_space_remaining;
    memcpy ( p_cTargetBuffer, (p_cSourceBuffer+target_space_remaining), source_bytes_remaining);
    *p_uintTargetBufferIndex = circular_add(uintSourceBufferLength, *p_uintTargetBufferIndex, uintTargetBufferLength); 
  }
}

static void lsm6dsr_ready_running_state(spi_device_handle_t *p_spi)
{
  // Turn off all interrupts
  lsm6dsr_write_register(p_spi, INT1_CTRL, 0x00);

  // Turn off XL
  lsm6dsr_write_register(p_spi, CTRL1_XL, 0x08);

  // Turn off Gyroscope
  lsm6dsr_write_register(p_spi, CTRL2_G , 0x04);

  // Set FIFO Watermark Interrupt
  lsm6dsr_write_register(p_spi, INT1_CTRL, 0x08);

  // Set XL to Max Data Rate (Add Gyro Later)
  lsm6dsr_write_register(p_spi, CTRL1_XL, 0xA8);
  // lsm6dsr_write_register(p_spi, CTRL2_G, 0xA4);

  // Start the FIFO
  lsm6dsr_FIFO_start(p_spi);
}