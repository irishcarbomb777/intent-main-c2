#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"

#include "sdkconfig.h"

#include "SensorStateMachineTask.h"
#include "NetworkStateMachineTask.h"
#include "lsm6dsr.h"
// #include "rtos_tasks.h"
#include "gpio.h"

#define watermark_threshold 300
typedef struct AppContext_t {
  // Task Handles
  TaskHandle_t xSensorStateMachineTaskHandle;
  TaskHandle_t xNetworkStateMachineTaskHandle;

  // Signal Semaphores
  SemaphoreHandle_t xConnectedClientsSemaphore;
  SemaphoreHandle_t xDataReadySemaphore;
  SemaphoreHandle_t xStartSetSemaphore;
  QueueHandle_t     xDataTransmitQueue;
  SemaphoreHandle_t xEndSetSemaphore;
  SemaphoreHandle_t xNetworkActiveSemaphore;
  SemaphoreHandle_t xNetworkInactiveSemaphore;

  // Shared Data Buffers
  char cConnectedClientsCount;

} AppContext_t;

void app_main(void)
{
  static const char *TAG = "Main Loop";
  QueueHandle_t xDataTransmitQueue = xQueueCreate(100, sizeof(char*));
  if (xDataTransmitQueue != NULL)
    printf("QUEUE SUCCESSSFULLY CREATED");
  // Initialize Data Structures
  AppContext_t ctxApp = {
    .xConnectedClientsSemaphore = xSemaphoreCreateBinary(),
    .xDataReadySemaphore = xSemaphoreCreateBinary(),
    .xStartSetSemaphore = xSemaphoreCreateBinary(),
    .xDataTransmitQueue = xDataTransmitQueue,
    .xEndSetSemaphore = xSemaphoreCreateBinary(),
    .xNetworkActiveSemaphore = xSemaphoreCreateBinary(),
    .xNetworkInactiveSemaphore = xSemaphoreCreateBinary(),
    .cConnectedClientsCount = 0,
  };
  
  SensorStateMachineTaskContext_t ctxSensorStateMachineTask = {
    .p_xConnectedClientsSemaphore = &(ctxApp.xConnectedClientsSemaphore),
    .p_xDataReadySemaphore = &(ctxApp.xDataReadySemaphore),
    .p_xStartSetSemaphore = &(ctxApp.xStartSetSemaphore),
    .p_xDataTransmitQueue = &(ctxApp.xDataTransmitQueue),
    .p_xEndSetSemaphore = &(ctxApp.xEndSetSemaphore),
    .p_xNetworkActiveSemaphore = &(ctxApp.xNetworkActiveSemaphore),
    .p_xNetworkInactiveSemaphore = &(ctxApp.xNetworkInactiveSemaphore),
    .p_cConnectedClientsCount = &(ctxApp.cConnectedClientsCount),
  };

  NetworkStateMachineTaskContext_t ctxNetworkStateMachineTask = {
    .p_xConnectedClientsSemaphore = (&ctxApp.xConnectedClientsSemaphore),
    .p_xStartSetSemaphore = &(ctxApp.xStartSetSemaphore),
    .p_xDataTransmitQueue = &(ctxApp.xDataTransmitQueue),
    .p_xEndSetSemaphore = &(ctxApp.xEndSetSemaphore),
    .p_xNetworkActiveSemaphore = &(ctxApp.xNetworkActiveSemaphore),
    .p_xNetworkInactiveSemaphore = &(ctxApp.xNetworkInactiveSemaphore),
    .p_cConnectedClientsCount = &(ctxApp.cConnectedClientsCount),
  };

  // Initialize Hardware
  ctxSensorStateMachineTask.p_spi = lsm6dsr_startup_routine();  // Get SPI Handle
  gpio_initialize_output_led();
  gpio_startup_routine(&(ctxApp.xDataReadySemaphore));

  // Initialize Tasks
  // Create Tasks on Core 0
  xTaskCreatePinnedToCore(SensorStateMachineTask, "Sensor Task", 16384, (void*)&ctxSensorStateMachineTask, 5, &(ctxApp.xSensorStateMachineTaskHandle), 0);

  // Create Tasks on Core 1
  xTaskCreatePinnedToCore(NetworkStateMachineTask, "Network Task", 16384, (void*)&ctxNetworkStateMachineTask, 5, &(ctxApp.xNetworkStateMachineTaskHandle), 1);

  while(1)
  {
    vTaskDelay(portMAX_DELAY);
  }
}
