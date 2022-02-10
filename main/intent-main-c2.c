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
#include "rtos_tasks.h"
#include "gpio.h"
#include "wifi.h"
#include "intent_mqtt.h"

#define watermark_threshold 300
typedef struct AppContext_t {
  // Task Handles
  TaskHandle_t xSensorStateMachineTaskHandle;
  TaskHandle_t xNetworkStateMachineTaskHandle;

  // Signal Semaphores
  SemaphoreHandle_t xConnectedClientsSemaphore;
  SemaphoreHandle_t xDataReadySemaphore;
  SemaphoreHandle_t xStartSetSemaphore;
  SemaphoreHandle_t xDataTransmitSemaphore;
  SemaphoreHandle_t xEndSetSemaphore;
  SemaphoreHandle_t xNetworkActiveSemaphore;
  SemaphoreHandle_t xNetworkInactiveSemaphore;

  // Shared Data Buffers
  char cConnectedClientsCount;
  char cDataTransmitBuffer[(watermark_threshold*7)+2];

} AppContext_t;

void app_main(void)
{
  // Initialize Data Structures
  AppContext_t ctxApp = {
    .xConnectedClientsSemaphore = xSemaphoreCreateBinary(),
    .xDataReadySemaphore = xSemaphoreCreateBinary(),
    .xStartSetSemaphore = xSemaphoreCreateBinary(),
    .xDataTransmitSemaphore = xSemaphoreCreateBinary(),
    .xEndSetSemaphore = xSemaphoreCreateBinary(),
    .xNetworkActiveSemaphore = xSemaphoreCreateBinary(),
    .xNetworkInactiveSemaphore = xSemaphoreCreateBinary(),
    .cConnectedClientsCount = 0,
    .cDataTransmitBuffer = memset(&(ctxApp.cDataTransmitBuffer), 0, sizeof(ctxApp.cDataTransmitBuffer)),
  };
  
  SensorStateMachineTaskContext_t ctxSensorStateMachineTask = {
    .p_xConnectedClientsSemaphore = &(ctxApp.xConnectedClientsSemaphore),
    .p_xDataReadySemaphore = &(ctxApp.xDataReadySemaphore),
    .p_xStartSetSemaphore = &(ctxApp.xStartSetSemaphore),
    .p_xDataTransmitSemaphore = &(ctxApp.xDataTransmitSemaphore),
    .p_xEndSetSemaphore = &(ctxApp.xEndSetSemaphore),
    .p_xNetworkActiveSemaphore = &(ctxApp.xNetworkActiveSemaphore),
    .p_xNetworkInactiveSemaphore = &(ctxApp.xNetworkInactiveSemaphore),
    .p_cConnectedClientsCount = &(ctxApp.cConnectedClientsCount),
    .p_cDataTransmitBuffer = &(ctxApp.cDataTransmitBuffer),
  };

  NetworkStateMachineTaskContext_t ctxNetworkStateMachineTask = {
    .p_xConnectedClientsSemaphore = (&ctxApp.xConnectedClientsSemaphore),
    .p_xStartSetSemaphore = &(ctxApp.xStartSetSemaphore),
    .p_xDataTransmitSemaphore = &(ctxApp.xDataTransmitSemaphore),
    .p_xEndSetSemaphore = &(ctxApp.xEndSetSemaphore),
    .p_xNetworkInactiveSemaphore = &(ctxApp.xNetworkActiveSemaphore),
    .p_xNetworkInactiveSemaphore = &(ctxApp.xNetworkInactiveSemaphore),
    .p_cConnectedClientsCount = &(ctxApp.cConnectedClientsCount),
    .p_cDataTransmitBuffer = &(ctxApp.cDataTransmitBuffer),
  };

  // Initialize Hardware
  ctxSensorStateMachineTask.p_spi = lsm6dsr_startup_routine();  // Get SPI Handle
  gpio_initialize_output_led();

  // Initialize Tasks
  // Create Tasks on Core 0
  xTaskCreatePinnedToCore(SensorStateMachineTask, "Sensor State Machine Task", 16384, (void*)&ctxSensorStateMachineTask, 5, &(ctxApp.xSensorStateMachineTaskHandle), 0);


  // Initialize Tasks
  rtos_tasks_shared_resources_t *p_rtos_task_resources = rtos_tasks_startup_routine(p_spi);
  gpio_startup_routine(p_rtos_task_resources);
  while(1)
  {
    vTaskDelay(portMAX_DELAY);
  }
}
