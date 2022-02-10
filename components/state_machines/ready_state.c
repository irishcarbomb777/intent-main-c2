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
#define ReadyState_XL_ODR 6667
#define RollbackBuffer_length 16384 // 14 bits 2.46 seconds

void vReadyState(ReadyStateContext_t *p_ctxReadyState)
{
  // Create Logging Tag
  static const char *TAG = "Ready State Log";

  // Create Data Buffers
  static BaseType_t xDataReadyResult;
  static char cFifoReadDataBuffer[(7*WATERMARK_THRESHOLD)+1]
  static char cRollbackBuffer[RollbackBuffer_length][7];
  static uint cRollbackBufferIndex = 0;

  // Parse Out Context Pointers
  SensorStateEnum_t *p_eSensorState               = p_ctxReadyState->p_eSensorState;
  spi_device_handle_t *p_spi                      = p_ctxReadyState->p_spi;
  SemaphoreHandle_t *p_xDataReadySemaphore        = p_ctxReadyState->p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xConnectedClientsSemaphore = p_ctxReadyState->p_xConnectedClientsSemaphore;
  char *p_cConnectedClientsCount                  = p_ctxReadyState->p_cConnectedClientsCount;

  // Set lsm6dsr to Ready State Mode
  lsm6dsr_ready_running_state(p_spi);

  // Begin Ready State Loop
  while (*p_eSensorState == SENSOR_READY)
  {
    xDataReadyResult = xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(1000));
    if (xDataReadyResult)
    {
      // Read Data from FIFO
      lsm6dsr_batch_read_fifo(p_spi, WATERMARK_THRESHOLD, &cFifoReadDataBuffer);
      
    } 
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

  // Set both XL and Gyroscope to MAX data rate
  lsm6dsr_write_register(p_spi, CTRL1_XL, 0xA8);
  lsm6dsr_write_register(p_spi, CTRL2_G, 0xA4);

  // Start the FIFO
  lsm6dsr_FIFO_start(p_spi);
}