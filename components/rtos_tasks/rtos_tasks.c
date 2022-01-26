#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

#include "intent_mqtt.h"
#include "rtos_tasks.h"
#include "lsm6dsr.h"
#include "wifi.h"


void sleep_task(void *arg)
{
  // Parse Activity Task Required Resources
  sleep_task_resources_t *p_sleep_task_resources = (sleep_task_resources_t*) arg;

  // Communication Protocols
  spi_device_handle_t *p_spi = p_sleep_task_resources->p_spi;

  // State Transition Semaphores
  SemaphoreHandle_t *p_xSleepSemaphore              = p_sleep_task_resources->p_xSleepSemaphore;
  SemaphoreHandle_t *p_xActiveSemaphore             = p_sleep_task_resources->p_xActiveSemaphore;
  SemaphoreHandle_t *p_xNetworkingInactiveSemaphore = p_sleep_task_resources->p_xNetworkingInactiveSemaphore;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore = p_sleep_task_resources->p_xDataReadySemaphore;

   
}

void active_task(void *arg)
{
  // Parse Data Transmit Task Required Resources
  active_task_resources_t *p_active_task_resources = (active_task_resources_t*) arg;

  // Communication Protocols
  spi_device_handle_t *p_spi = p_active_task_resources->p_spi;
  
  // State Transition Semaphores
  SemaphoreHandle_t *p_xActiveSemaphore           = p_active_task_resources->p_xActiveSemaphore;
  SemaphoreHandle_t *p_xSleepSemaphore            = p_active_task_resources->p_xSleepSemaphore;
  SemaphoreHandle_t *p_xReadySemaphore            = p_active_task_resources->p_xReadySemaphore;
  SemaphoreHandle_t *p_xNetworkingActiveSemaphore = p_active_task_resources->p_xNetworkingActiveSemaphore;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore        = p_active_task_resources->p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xConnectedClientsSemaphore = p_active_task_resources->p_xConnectedClientsSemaphore;

  // Shared Data Buffers
  char *p_connected_clients_count = p_active_task_resources->p_connected_clients_count;
}

void ready_task(void *arg)
{
  // Parse Activity Task Required Resources
  ready_task_resources_t *p_ready_task_resources = (ready_task_resources_t*) arg;

  // Communication Protocols
  spi_device_handle_t *p_spi = p_ready_task_resources->p_spi;

  // State Transition Semaphores
  SemaphoreHandle_t *p_xReadySemaphore   = p_ready_task_resources->p_xReadySemaphore;
  SemaphoreHandle_t *p_xActiveSemaphore  = p_ready_task_resources->p_xActiveSemaphore;
  SemaphoreHandle_t *p_xRunningSemaphore = p_ready_task_resources->p_xRunningSemaphore;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore        = p_ready_task_resources->p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xConnectedClientsSemaphore = p_ready_task_resources->p_xConnectedClientsSemaphore;

  // Shared Data Buffers
  char *p_connected_clients_count = p_ready_task_resources->p_connected_clients_count;

}

void running_task(void *arg)
{
  // Parse Activity Task Required Resources
  running_task_resources_t *p_running_task_resources = (running_task_resources_t*) arg; 

  // Communication Protocols
  spi_device_handle_t *p_spi = p_running_task_resources->p_spi;

  // State Transition Semaphores
  SemaphoreHandle_t *p_xRunningSemaphore = p_running_task_resources->p_xRunningSemaphore;
  SemaphoreHandle_t *p_xReadySemaphore   = p_running_task_resources->p_xReadySemaphore;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore    = p_running_task_resources->p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xStartSetSemaphore     = p_running_task_resources->p_xStartSetSemaphore;
  SemaphoreHandle_t *p_xDataTransmitSemaphore = p_running_task_resources->p_xDataTransmitSemaphore;
  SemaphoreHandle_t *p_xEndSetSemaphore       = p_running_task_resources->p_xEndSetSemaphore;

  // Shared Data Buffers
  char *p_data_transmit_buffer = p_running_task_resources->p_data_transmit_buffer;

}


void networking_active_task(void *arg)
{
  // Parse Activity Task Required Resources
  networking_active_task_resources_t *p_networking_active_task_resources = (networking_active_task_resources_t*) arg; 

  // State Transition Semaphores
  SemaphoreHandle_t *p_xNetworkingActiveSemaphore   = p_networking_active_task_resources->p_xNetworkingActiveSemaphore;
  SemaphoreHandle_t *p_xNetworkingInactiveSemaphore = p_networking_active_task_resources->p_xNetworkingInactiveSemaphore;

  // Signal Semaphores
  SemaphoreHandle_t *p_xConnectedClientsSemaphore = p_networking_active_task_resources->p_xConnectedClientsSemaphore;
  SemaphoreHandle_t *p_xStartSetSemaphore         = p_networking_active_task_resources->p_xStartSetSemaphore;
  SemaphoreHandle_t *p_xDataTransmitSemaphore     = p_networking_active_task_resources->p_xDataTransmitSemaphore;
  SemaphoreHandle_t *p_xEndSetSemaphore           = p_networking_active_task_resources->p_xEndSetSemaphore;

  // Shared Data Buffers
  char *p_connected_clients_count = p_networking_active_task_resources->p_connected_clients_count;
  char *p_data_transmit_buffer    = p_networking_active_task_resources->p_data_transmit_buffer;

}

void networking_inactive_task(void *arg)
{
  // Parse Activity Task Required Resources
  networking_inactive_task_resources_t *p_networking_inactive_task_resources = (networking_inactive_task_resources_t*) arg;

  // State Transition Semaphores
  SemaphoreHandle_t *p_xNetworkingInactiveSemaphore = p_networking_inactive_task_resources->p_xNetworkingInactiveSemaphore;
  SemaphoreHandle_t *p_xNetworkingActiveSemaphore   = p_networking_inactive_task_resources->p_xNetworkingActiveSemaphore;

}

/********************************************************
************ Startup Routine Code ***********************
********************************************************/

rtos_tasks_shared_resources_t * rtos_tasks_startup_routine(spi_device_handle_t *p_spi)
{
  // Initialize Task Handles
  TaskHandle_t *p_xSleepTaskHandle = (TaskHandle_t*)malloc(sizeof(TaskHandle_t));
  TaskHandle_t *p_xActiveTaskHandle = (TaskHandle_t*)malloc(sizeof(TaskHandle_t));
  TaskHandle_t *p_xReadyTaskHandle = (TaskHandle_t*)malloc(sizeof(TaskHandle_t));
  TaskHandle_t *p_xRunningTaskHandle = (TaskHandle_t*)malloc(sizeof(TaskHandle_t));
  TaskHandle_t *p_xNetworkingActiveTaskHandle = (TaskHandle_t*)malloc(sizeof(TaskHandle_t));
  TaskHandle_t *p_xNetworkingInactiveTaskHandle = (TaskHandle_t*)malloc(sizeof(TaskHandle_t));

  // Initialize State Transition Semaphores
  SemaphoreHandle_t *p_xSleepSemaphore = (SemaphoreHandle_t*)malloc(sizeof(SemaphoreHandle_t));
  SemaphoreHandle_t *p_xActiveSemaphore = (SemaphoreHandle_t*)malloc(sizeof(SemaphoreHandle_t));
  SemaphoreHandle_t *p_xReadySemaphore = (SemaphoreHandle_t*)malloc(sizeof(SemaphoreHandle_t));
  SemaphoreHandle_t *p_xRunningSemaphore = (SemaphoreHandle_t*)malloc(sizeof(SemaphoreHandle_t));
  SemaphoreHandle_t *p_xNetworkingActiveSemaphore = (SemaphoreHandle_t*)malloc(sizeof(SemaphoreHandle_t));
  SemaphoreHandle_t *p_xNetworkingInactiveSemaphore = (SemaphoreHandle_t*)malloc(sizeof(SemaphoreHandle_t));
  *p_xSleepSemaphore              = xSemaphoreCreateBinary();   // Initialize to 1
  *p_xActiveSemaphore             = xSemaphoreCreateBinary();   // Initialize to 0
  xSemaphoreTake(*p_xActiveSemaphore, 0);
  *p_xReadySemaphore              = xSemaphoreCreateBinary();   // Initialize to 0
  xSemaphoreTake(*p_xReadySemaphore, 0);
  *p_xRunningSemaphore            = xSemaphoreCreateBinary();   // Initialize to 0
  xSemaphoreTake(*p_xRunningSemaphore, 0);
  *p_xNetworkingActiveSemaphore   = xSemaphoreCreateBinary();   // Initialize to 0
  xSemaphoreTake(*p_xNetworkingActiveSemaphore, 0);
  *p_xNetworkingInactiveSemaphore = xSemaphoreCreateBinary();   // Initialize to 1

  // Initialize Signal Semaphores
  SemaphoreHandle_t *p_xConnectedClientsSemaphore = (SemaphoreHandle_t*)malloc(sizeof(SemaphoreHandle_t));
  SemaphoreHandle_t *p_xDataReadySemaphore        = (SemaphoreHandle_t*)malloc(sizeof(SemaphoreHandle_t)); 
  SemaphoreHandle_t *p_xStartSetSemaphore         = (SemaphoreHandle_t*)malloc(sizeof(SemaphoreHandle_t)); 
  SemaphoreHandle_t *p_xDataTransmitSemaphore     = (SemaphoreHandle_t*)malloc(sizeof(SemaphoreHandle_t)); 
  SemaphoreHandle_t *p_xEndSetSemaphore           = (SemaphoreHandle_t*)malloc(sizeof(SemaphoreHandle_t));  
  *p_xConnectedClientsSemaphore = xSemaphoreCreateBinary();     // Initialize to 0
  xSemaphoreTake(*p_xConnectedClientsSemaphore, 0);
  *p_xDataReadySemaphore        = xSemaphoreCreateBinary();     // Initialize to 0
  xSemaphoreTake(*p_xDataReadySemaphore, 0);
  *p_xStartSetSemaphore         = xSemaphoreCreateBinary();     // Initialize to 0
  xSemaphoreTake(*p_xStartSetSemaphore, 0);
  *p_xDataTransmitSemaphore     = xSemaphoreCreateBinary();     // Initialize to 0
  xSemaphoreTake(*p_xDataTransmitSemaphore, 0);
  *p_xEndSetSemaphore           = xSemaphoreCreateBinary();     // Initialize to 0
  xSemaphoreTake(*p_xEndSetSemaphore , 0);

  // Initialize shared data buffers
  char watermark_threshold_byte_low  = lsm6dsr_read_register(p_spi, FIFO_CTRL1);
  char watermark_threshold_byte_high = lsm6dsr_read_register(p_spi, FIFO_CTRL2);
  int watermark_threshold = 0x00 | ((watermark_threshold_byte_high << 8) | watermark_threshold_byte_low);
  char *p_data_transmit_buffer = (char*) malloc(sizeof(char)*7*watermark_threshold + (sizeof(char)*2));
  char *p_connected_clients_count;

  // Initialize Sleep Task Resources
  sleep_task_resources_t *p_sleep_task_resources = (sleep_task_resources_t*)malloc(sizeof(sleep_task_resources_t));
  p_sleep_task_resources->p_spi = p_spi;
  p_sleep_task_resources->p_xSleepSemaphore = p_xSleepSemaphore;
  p_sleep_task_resources->p_xActiveSemaphore = p_xActiveSemaphore;
  p_sleep_task_resources->p_xNetworkingInactiveSemaphore = p_xNetworkingInactiveSemaphore;
  p_sleep_task_resources->p_xDataReadySemaphore = p_xDataReadySemaphore;

  // Initialize Active Task Resources
  active_task_resources_t *p_active_task_resources = (active_task_resources_t*)malloc(sizeof(active_task_resources_t));
  p_active_task_resources->p_spi = p_spi;
  p_active_task_resources->p_xActiveSemaphore = p_xActiveSemaphore;
  p_active_task_resources->p_xSleepSemaphore = p_xSleepSemaphore;
  p_active_task_resources->p_xReadySemaphore = p_xReadySemaphore;
  p_active_task_resources->p_xNetworkingActiveSemaphore = p_xNetworkingActiveSemaphore;
  p_active_task_resources->p_xDataReadySemaphore = p_xDataReadySemaphore;
  p_active_task_resources->p_xConnectedClientsSemaphore = p_xConnectedClientsSemaphore;

  // Initialize Ready Task Resources
  ready_task_resources_t *p_ready_task_resources = (ready_task_resources_t*)malloc(sizeof(ready_task_resources_t));
  p_ready_task_resources->p_spi = p_spi;
  p_ready_task_resources->p_xReadySemaphore = p_xReadySemaphore;
  p_ready_task_resources->p_xActiveSemaphore = p_xActiveSemaphore;
  p_ready_task_resources->p_xRunningSemaphore = p_xRunningSemaphore;
  p_ready_task_resources->p_xDataReadySemaphore = p_xDataReadySemaphore;
  p_ready_task_resources->p_xConnectedClientsSemaphore = p_xConnectedClientsSemaphore;
  p_ready_task_resources->p_connected_clients_count = p_connected_clients_count;

  // Initialize Running Task Resources
  running_task_resources_t *p_running_task_resources = (running_task_resources_t*)malloc(sizeof(running_task_resources_t));
  p_running_task_resources->p_spi = p_spi;
  p_running_task_resources->p_xRunningSemaphore = p_xRunningSemaphore;
  p_running_task_resources->p_xReadySemaphore = p_xReadySemaphore;
  p_running_task_resources->p_xDataReadySemaphore = p_xDataReadySemaphore;
  p_running_task_resources->p_xStartSetSemaphore = p_xStartSetSemaphore;
  p_running_task_resources->p_xDataTransmitSemaphore = p_xDataTransmitSemaphore;
  p_running_task_resources->p_xEndSetSemaphore = p_xEndSetSemaphore;
  p_running_task_resources->p_data_transmit_buffer = p_data_transmit_buffer;

  // Initialize Networking Active Task Resources
  networking_active_task_resources_t *p_networking_active_task_resources = (networking_active_task_resources_t*)malloc(sizeof(networking_active_task_resources_t));
  p_networking_active_task_resources->p_xNetworkingActiveSemaphore = p_xNetworkingActiveSemaphore;
  p_networking_active_task_resources->p_xNetworkingInactiveSemaphore = p_xNetworkingInactiveSemaphore;
  p_networking_active_task_resources->p_xConnectedClientsSemaphore = p_xConnectedClientsSemaphore;
  p_networking_active_task_resources->p_xStartSetSemaphore = p_xStartSetSemaphore;
  p_networking_active_task_resources->p_xDataTransmitSemaphore = p_xDataTransmitSemaphore;
  p_networking_active_task_resources->p_xEndSetSemaphore = p_xEndSetSemaphore;
  p_networking_active_task_resources->p_connected_clients_count = p_connected_clients_count;
  p_networking_active_task_resources->p_data_transmit_buffer = p_data_transmit_buffer;

  // Initialize Networking Inactive Task Resources
  networking_inactive_task_resources_t *p_networking_inactive_task_resources = (networking_inactive_task_resources_t*)malloc(sizeof(networking_inactive_task_resources_t));
  p_networking_inactive_task_resources->p_xNetworkingInactiveSemaphore = p_xNetworkingInactiveSemaphore;
  p_networking_inactive_task_resources->p_xNetworkingActiveSemaphore = p_xNetworkingActiveSemaphore;

  // Create Tasks on Core 0
  xTaskCreatePinnedToCore(sleep_task,   "Sleep Task",   2048,  (void*)p_sleep_task_resources,   1, p_xSleepTaskHandle,   0);
  xTaskCreatePinnedToCore(active_task,  "Active Task",  2048,  (void*)p_active_task_resources,  2, p_xActiveTaskHandle,  0);
  xTaskCreatePinnedToCore(ready_task,   "Ready Task",   2048,  (void*)p_ready_task_resources,   3, p_xReadyTaskHandle,   0);
  xTaskCreatePinnedToCore(running_task, "Running Task", 16384, (void*)p_running_task_resources, 4, p_xRunningTaskHandle, 0);

  // Create Tasks on Core 1
  xTaskCreatePinnedToCore(networking_active_task,   "Networking Active Task",  16384, (void*)p_networking_active_task_resources,   2, p_xNetworkingActiveTaskHandle,   1);
  xTaskCreatePinnedToCore(networking_inactive_task, "Networking Inactive Task", 2048, (void*)p_networking_inactive_task_resources, 1, p_xNetworkingInactiveTaskHandle, 1);

  // Initialize  to return task handles to main
  rtos_tasks_shared_resources_t *p_rtos_tasks_shared_resources = (rtos_tasks_shared_resources_t*)malloc(sizeof(rtos_tasks_shared_resources_t));
  p_rtos_tasks_shared_resources->p_xSleepTaskHandle = p_xSleepTaskHandle;
  p_rtos_tasks_shared_resources->p_xActiveTaskHandle = p_xActiveTaskHandle;
  p_rtos_tasks_shared_resources->p_xReadyTaskHandle = p_xReadyTaskHandle;
  p_rtos_tasks_shared_resources->p_xRunningTaskHandle = p_xRunningTaskHandle;
  p_rtos_tasks_shared_resources->p_xNetworkingActiveTaskHandle = p_xNetworkingActiveTaskHandle;
  p_rtos_tasks_shared_resources->p_xNetworkingInactiveTaskHandle = p_xNetworkingInactiveTaskHandle;
  p_rtos_tasks_shared_resources->p_xSleepSemaphore = p_xSleepSemaphore;
  p_rtos_tasks_shared_resources->p_xActiveSemaphore = p_xActiveSemaphore;
  p_rtos_tasks_shared_resources->p_xReadySemaphore = p_xReadySemaphore;
  p_rtos_tasks_shared_resources->p_xRunningSemaphore = p_xRunningSemaphore;
  p_rtos_tasks_shared_resources->p_xNetworkingActiveSemaphore = p_xNetworkingActiveSemaphore;
  p_rtos_tasks_shared_resources->p_xNetworkingInactiveSemaphore = p_xNetworkingInactiveSemaphore;
  p_rtos_tasks_shared_resources->p_xConnectedClientsSemaphore = p_xConnectedClientsSemaphore;
  p_rtos_tasks_shared_resources->p_xDataReadySemaphore = p_xDataReadySemaphore;
  p_rtos_tasks_shared_resources->p_xStartSetSemaphore = p_xStartSetSemaphore;
  p_rtos_tasks_shared_resources->p_xDataTransmitSemaphore = p_xDataTransmitSemaphore;
  p_rtos_tasks_shared_resources->p_xEndSetSemaphore = p_xEndSetSemaphore;
  p_rtos_tasks_shared_resources->p_connected_clients_count = p_connected_clients_count;
  p_rtos_tasks_shared_resources->p_data_transmit_buffer = p_data_transmit_buffer;
  return p_rtos_tasks_shared_resources;
}
