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

#include "intent_mqtt.h"
#include "rtos_tasks.h"
#include "lsm6dsr.h"
#include "wifi.h"
#include "gpio.h"
#include "state_machine.h"


void state_machine_task(void *arg)
{
  // Create Required Variables
  sensor_state_enum_t sensor_state = SENSOR_SLEEP;

  // Parse Out State Machine Resources
  state_machine_task_resources_t *p_state_machine_task_resources = (state_machine_task_resources_t*) arg;

  // Create Sleep State Resources Struct
  sleep_state_resources_t sleep_state_resources = {
    .p_spi = p_state_machine_task_resources->p_spi,
    .p_xDataReadySemaphore = p_state_machine_task_resources->p_xDataReadySemaphore,
    .p_xNetworkInactiveSemaphore = p_state_machine_task_resources->p_xNetworkInactiveSemaphore,
  };

  // Create Active State Resources Struct
  active_state_resources_t active_state_resources = {
    .p_spi = p_state_machine_task_resources->p_spi,
    .p_xDataReadySemaphore = p_state_machine_task_resources->p_xDataReadySemaphore,
    .p_xConnectedClientsSemaphore = p_state_machine_task_resources->p_xConnectedClientsSemaphore,
    .p_xNetworkActiveSemaphore = p_state_machine_task_resources->p_xNetworkActiveSemaphore,
    .p_connected_clients_count = p_state_machine_task_resources->p_connected_clients_count,
  };

  // Create Ready State Resources Struct
  ready_state_resources_t ready_state_resources = {
    .p_spi = p_state_machine_task_resources->p_spi,
    .p_xDataReadySemaphore = p_state_machine_task_resources->p_xDataReadySemaphore,
    .p_xConnectedClientsSemaphore = p_state_machine_task_resources->p_xConnectedClientsSemaphore,
    .p_connected_clients_count = p_state_machine_task_resources->p_connected_clients_count,
  };

  // Create Running State Resources Struct
  running_state_resources_t running_state_resources = {
    .p_spi = p_state_machine_task_resources->p_spi,
    .p_xDataReadySemaphore = p_state_machine_task_resources->p_xDataReadySemaphore,
    .p_xDataTransmitSemaphore = p_state_machine_task_resources->p_xDataTransmitSemaphore,
    .p_xStartSetSemaphore = p_state_machine_task_resources->p_xStartSetSemaphore,
    .p_xEndSetSemaphore = p_state_machine_task_resources->p_xEndSetSemaphore,
    .p_data_transmit_buffer = p_state_machine_task_resources->p_data_transmit_buffer,
  };

  while (1)
  {
    switch (sensor_state)
    {
      case SENSOR_SLEEP:
        sleep_state(&sleep_state_resources, &sensor_state); 
        break;
      case SENSOR_ACTIVE:
        active_state(&active_state_resources, &sensor_state);
        break;
      case SENSOR_READY:
        ready_state(&ready_state_resources, &sensor_state);
        break;
      case SENSOR_RUNNING:
        running_state(&running_state_resources, &sensor_state);
        break;
      
      default:
        break;
    }
  }
}

void network_task(void *arg)
{
  // Create Logging Tag
  static const char *TAG = "Network Task Log";

  // Initialize Networking State
  network_state_enum_t network_state = NETWORK_INACTIVE;

  // Parse Activity Task Required Resources
  network_task_resources_t *p_network_task_resources = (network_task_resources_t*) arg; 

  // Initialize ESP Networking Resources
  wifi_startup_routine();
  ESP_LOGI(TAG, "Wifi End");
  esp_mqtt_client_handle_t *p_mqtt_client = mqtt_startup_routine(p_network_task_resources->p_xConnectedClientsSemaphore, p_network_task_resources->p_connected_clients_count);
  ESP_LOGI(TAG, "MQTT End");
  // Create Network Active State Resources
  network_active_state_resources_t network_active_state_resources = {
    .p_mqtt_client = p_mqtt_client,
    .p_xNetworkInactiveSemaphore = p_network_task_resources->p_xNetworkInactiveSemaphore,
    .p_xConnectedClientsSemaphore = p_network_task_resources->p_xConnectedClientsSemaphore,
    .p_xStartSetSemaphore = p_network_task_resources->p_xStartSetSemaphore,
    .p_xDataTransmitSemaphore = p_network_task_resources->p_xDataTransmitSemaphore,
    .p_xEndSetSemaphore = p_network_task_resources->p_xEndSetSemaphore,
    .p_connected_clients_count = p_network_task_resources->p_connected_clients_count,
    .p_data_transmit_buffer = p_network_task_resources->p_data_transmit_buffer,
  };

  // Create Network Inactive State Resources
  network_inactive_state_resources_t network_inactive_state_resources = {
    .p_mqtt_client = p_mqtt_client,
    .p_xNetworkActiveSemaphore = p_network_task_resources->p_xNetworkActiveSemaphore,
  };

  // Clear Inactive Semaphore
  xSemaphoreTake(*(p_network_task_resources->p_xNetworkInactiveSemaphore), 0);
  while (1)
  {
    switch (network_state)
    {
      case NETWORK_INACTIVE:
        network_inactive_state(&network_inactive_state_resources, &network_state);
        break;
      case NETWORK_ACTIVE:
        network_active_state(&network_active_state_resources, &network_state);
        break;
      default:
        break;
    }
  }
}


/********************************************************
************ Startup Routine Code ***********************
********************************************************/

rtos_tasks_shared_resources_t * rtos_tasks_startup_routine(spi_device_handle_t *p_spi)
{
  // Initialize Task Handles
  TaskHandle_t *p_xStateMachineTaskHandle = (TaskHandle_t*)malloc(sizeof(TaskHandle_t));
  TaskHandle_t *p_xNetworkTaskHandle = (TaskHandle_t*)malloc(sizeof(TaskHandle_t));

  // Initialize Signal Semaphores
  SemaphoreHandle_t *p_xConnectedClientsSemaphore = (SemaphoreHandle_t*)malloc(sizeof(SemaphoreHandle_t));
  SemaphoreHandle_t *p_xDataReadySemaphore        = (SemaphoreHandle_t*)malloc(sizeof(SemaphoreHandle_t)); 
  SemaphoreHandle_t *p_xStartSetSemaphore         = (SemaphoreHandle_t*)malloc(sizeof(SemaphoreHandle_t)); 
  SemaphoreHandle_t *p_xDataTransmitSemaphore     = (SemaphoreHandle_t*)malloc(sizeof(SemaphoreHandle_t)); 
  SemaphoreHandle_t *p_xEndSetSemaphore           = (SemaphoreHandle_t*)malloc(sizeof(SemaphoreHandle_t));
  SemaphoreHandle_t *p_xNetworkActiveSemaphore = (SemaphoreHandle_t*)malloc(sizeof(SemaphoreHandle_t));
  SemaphoreHandle_t *p_xNetworkInactiveSemaphore = (SemaphoreHandle_t*)malloc(sizeof(SemaphoreHandle_t));

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
  *p_xNetworkActiveSemaphore   = xSemaphoreCreateBinary();   // Initialize to 0
  xSemaphoreTake(*p_xNetworkActiveSemaphore, 0);
  *p_xNetworkInactiveSemaphore = xSemaphoreCreateBinary();   // Initialize to 0
  xSemaphoreTake(*p_xNetworkInactiveSemaphore, 0);
  // Initialize shared data buffers
  char watermark_threshold_byte_low  = lsm6dsr_read_register(p_spi, FIFO_CTRL1);
  char watermark_threshold_byte_high = lsm6dsr_read_register(p_spi, FIFO_CTRL2);
  int watermark_threshold = 0x00 | ((watermark_threshold_byte_high << 8) | watermark_threshold_byte_low);
  char *p_data_transmit_buffer = (char*) malloc(sizeof(char)*7*watermark_threshold + (sizeof(char)*2));
  char *p_connected_clients_count = (char*) malloc(sizeof(char));
  *p_connected_clients_count = 0;

  // Initialize State Machine Task Resources
  state_machine_task_resources_t *p_state_machine_task_resources = (state_machine_task_resources_t*)malloc(sizeof(state_machine_task_resources_t));
  p_state_machine_task_resources->p_spi = p_spi;
  p_state_machine_task_resources->p_xDataReadySemaphore = p_xDataReadySemaphore;
  p_state_machine_task_resources->p_xNetworkInactiveSemaphore = p_xNetworkInactiveSemaphore;
  p_state_machine_task_resources->p_xConnectedClientsSemaphore = p_xConnectedClientsSemaphore;
  p_state_machine_task_resources->p_xNetworkActiveSemaphore = p_xNetworkActiveSemaphore;
  p_state_machine_task_resources->p_xDataTransmitSemaphore = p_xDataTransmitSemaphore;
  p_state_machine_task_resources->p_xStartSetSemaphore = p_xStartSetSemaphore;
  p_state_machine_task_resources->p_xEndSetSemaphore = p_xEndSetSemaphore;
  p_state_machine_task_resources->p_data_transmit_buffer = p_data_transmit_buffer;
  p_state_machine_task_resources->p_connected_clients_count = p_connected_clients_count;


  // Initialize Network Active Task Resources
  network_task_resources_t *p_network_task_resources = (network_task_resources_t*)malloc(sizeof(network_task_resources_t));
  p_network_task_resources->p_xNetworkActiveSemaphore = p_xNetworkActiveSemaphore;
  p_network_task_resources->p_xNetworkInactiveSemaphore = p_xNetworkInactiveSemaphore;
  p_network_task_resources->p_xConnectedClientsSemaphore = p_xConnectedClientsSemaphore;
  p_network_task_resources->p_xStartSetSemaphore = p_xStartSetSemaphore;
  p_network_task_resources->p_xDataTransmitSemaphore = p_xDataTransmitSemaphore;
  p_network_task_resources->p_xEndSetSemaphore = p_xEndSetSemaphore;
  p_network_task_resources->p_connected_clients_count = p_connected_clients_count;
  p_network_task_resources->p_data_transmit_buffer = p_data_transmit_buffer;


  // Create Tasks on Core 0
  xTaskCreatePinnedToCore(state_machine_task, "State Machine Task", 16384, (void*)p_state_machine_task_resources, 5, p_xStateMachineTaskHandle, 0);

  // Create Tasks on Core 1
  xTaskCreatePinnedToCore(network_task,    "Network Task",    16384, (void*)p_network_task_resources,    2, p_xNetworkTaskHandle,   1);

  // Initialize to return task handles to main
  rtos_tasks_shared_resources_t *p_rtos_tasks_shared_resources = (rtos_tasks_shared_resources_t*)malloc(sizeof(rtos_tasks_shared_resources_t));
  p_rtos_tasks_shared_resources->p_xStateMachineTaskHandle = p_xStateMachineTaskHandle;
  p_rtos_tasks_shared_resources->p_xNetworkTaskHandle = p_xNetworkTaskHandle;
  p_rtos_tasks_shared_resources->p_xNetworkActiveSemaphore = p_xNetworkActiveSemaphore;
  p_rtos_tasks_shared_resources->p_xNetworkInactiveSemaphore = p_xNetworkInactiveSemaphore;
  p_rtos_tasks_shared_resources->p_xConnectedClientsSemaphore = p_xConnectedClientsSemaphore;
  p_rtos_tasks_shared_resources->p_xDataReadySemaphore = p_xDataReadySemaphore;
  p_rtos_tasks_shared_resources->p_xStartSetSemaphore = p_xStartSetSemaphore;
  p_rtos_tasks_shared_resources->p_xDataTransmitSemaphore = p_xDataTransmitSemaphore;
  p_rtos_tasks_shared_resources->p_xEndSetSemaphore = p_xEndSetSemaphore;
  p_rtos_tasks_shared_resources->p_connected_clients_count = p_connected_clients_count;
  p_rtos_tasks_shared_resources->p_data_transmit_buffer = p_data_transmit_buffer;
  return p_rtos_tasks_shared_resources;
}
