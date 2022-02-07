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
  sensor_state_enum_t state = SLEEP;

  // Parse Out State Machine Resources
  state_machine_task_resources_t *p_state_machine_task_resources = (state_machine_task_resources_t*) arg;

  // Create Sleep State Resources Struct
  sleep_state_resources_t sleep_state_resources = {
    .p_spi = p_state_machine_task_resources->p_spi,
    .p_xDataReadySemaphore = p_state_machine_task_resources->p_xDataReadySemaphore,
    .p_xNetworkingInactiveSemaphore = p_state_machine_task_resources->p_xNetworkingInactiveSemaphore,
  };

  // Create Active State Resources Struct
  active_state_resources_t active_state_resources = {
    .p_spi = p_state_machine_task_resources->p_spi,
    .p_xDataReadySemaphore = p_state_machine_task_resources->p_xDataReadySemaphore,
    .p_xConnectedClientsSemaphore = p_state_machine_task_resources->p_xConnectedClientsSemaphore,
    .p_xNetworkingActiveSemaphore = p_state_machine_task_resources->p_xNetworkingActiveSemaphore,
    .p_connected_clients_count = p_state_machine_task_resources->p_connected_clients_count,
  };

  while (1)
  {
    switch (state)
    {
      case SLEEP:
        sleep_state(&sleep_state_resources, &state); 
        break;
      case ACTIVE:
        active_state(&active_state_resources, &state);
        break;
      case READY:
        // code
        break;
      case RUNNING:
        //code
        break;
      
      default:
        break;
    }
  }
}

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

  // Create Logging Tag
  static const char *TAG = "Sleep Task Log";

  // Create Required Variables
  bool sleep = false;
  BaseType_t xDataReadyResult;
  char read_data_buffer[7];
  int16_t acc_xyz_data_buffer[3];
  double acc_magnitude;
  char N = 52;
  double acc_mag_data_window[N];
  char active_sample_count = 0;
  char active_count_threshold = 2;
  double acc_activity_threshold = 100;

  for ( ;; )
  {
    // Take sleep semaphore
    xSemaphoreTake(*p_xSleepSemaphore, portMAX_DELAY);
    gpio_set_level(LED_RED_PIN, 1);
    
    sleep = true;
    // Give Networking Inactive Semaphore
    xSemaphoreGive(*p_xNetworkingInactiveSemaphore);

    // Set lsm6dsr to low power mode
    lsm6dsr_enter_sleep_active_state(p_spi);

    // Fill the Acceleration Magnitude Data Window
    for ( int i=0; i < N; i++ )
    {
      // Wait for Data to be ready
      xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(1000));
      // Read Data
      lsm6dsr_read_single_xl_measurement(p_spi, read_data_buffer);
      // Parse Data Read Buffer
      lsm6dsr_parse_read_data_buffer(read_data_buffer, acc_xyz_data_buffer);
      // Calculate Magnitude
      acc_magnitude = (pow( (pow( (double) acc_xyz_data_buffer[0], 2) + pow((double) acc_xyz_data_buffer[1], 2) + pow((double) acc_xyz_data_buffer[2], 2)), 0.5 ));
      acc_magnitude = abs(acc_magnitude - 8192);
      // Append to Acceleration Magnitude Window
      acc_mag_data_window[i] = acc_magnitude;
      if (acc_magnitude > acc_activity_threshold)
      {
        active_sample_count++;
      }
    }

    // Check if activity was found while filling data window
    if (active_sample_count > active_count_threshold)
    {
      // Reset Data Buffers and give Semaphore to Active State
      sleep = false;
      memset(&read_data_buffer, 0, sizeof(read_data_buffer));
      memset(&acc_xyz_data_buffer, 0, sizeof(acc_xyz_data_buffer));
      memset(&acc_mag_data_window, 0, sizeof(acc_mag_data_window));
      active_sample_count = 0;
      gpio_set_level(LED_RED_PIN, 0);
      xSemaphoreGive(*p_xActiveSemaphore);
    }

    // Begin Detection Loop
    while (sleep)
    {
      xDataReadyResult = xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(1000));
      if (xDataReadyResult)
      {
        lsm6dsr_read_single_xl_measurement(p_spi, read_data_buffer);

        // Parse Data Read Buffer
        lsm6dsr_parse_read_data_buffer(read_data_buffer, acc_xyz_data_buffer);

        // Calculate Magnitude
        acc_magnitude = (pow( (pow( (double) acc_xyz_data_buffer[0], 2) + pow((double) acc_xyz_data_buffer[1], 2) + pow((double) acc_xyz_data_buffer[2], 2)), 0.5 ));
        acc_magnitude = abs(acc_magnitude - 8192);

        // Check the first and last data point and modify active sample count
        if (acc_magnitude > acc_activity_threshold)
        {
          active_sample_count++;
        }
        if (acc_mag_data_window[0] < acc_activity_threshold && (active_sample_count > 0))
          active_sample_count--;

        // Shift all data in buffer to left
        for (int i=0; i < (N-1); i++)
          acc_mag_data_window[i] = acc_mag_data_window[i+1];

        // Set new sample to last element in buffer
        acc_mag_data_window[N-1] = acc_magnitude;
        
        if (active_sample_count > active_count_threshold)
        {
          // Reset Data Buffers and give Semaphore to Active State
          sleep = false;
          memset(&read_data_buffer, 0, sizeof(read_data_buffer));
          memset(&acc_xyz_data_buffer, 0, sizeof(acc_xyz_data_buffer));
          memset(&acc_mag_data_window, 0, sizeof(acc_mag_data_window));
          active_sample_count = 0;
          gpio_set_level(LED_RED_PIN, 0);
          xSemaphoreGive(*p_xActiveSemaphore);
        }
      }
      else
      {
        lsm6dsr_read_single_xl_measurement(p_spi, read_data_buffer);
        ESP_LOGI(TAG, "Interrupt Timed out(Line 150)");
      }
    }
  }
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

  // Create Logging Tag
  static const char *TAG = "Active Task Log";

  // Create Required Variables
  bool active = false;
  BaseType_t xDataReadyResult;
  BaseType_t xConnectedClientsResult;
  char read_data_buffer[7];
  int16_t acc_xyz_data_buffer[3];
  double acc_magnitude;
  char N = 52;
  double acc_mag_data_window[N];
  char active_sample_count = 0;
  double acc_activity_threshold = 100;
  char active_sample_count_threshold = 2;
  unsigned int inactive_window_count = 0;
  unsigned int inactive_window_count_threshold = N*5; // Wait for 4 minutes

  

  for ( ;; )
  {
    // Take Semaphore
    xSemaphoreTake(*p_xActiveSemaphore, portMAX_DELAY);
    gpio_set_level(LED_BLUE_PIN, 1);
    active = true;

    // Give Networking Active Semaphore
    xSemaphoreGive(*p_xNetworkingActiveSemaphore);

    // Set lsm6dsr to low power mode
    lsm6dsr_enter_sleep_active_state(p_spi);

    // Fill the Acceleration Magnitude Data Window
    for ( int i=0; i < N; i++ )
    {
      // Wait for Data to be ready
      xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(1000));

      // Read Data
      lsm6dsr_read_single_xl_measurement(p_spi, read_data_buffer);

      // Parse Data Read Buffer
      lsm6dsr_parse_read_data_buffer(read_data_buffer, acc_xyz_data_buffer);

      // Calculate Magnitude
      acc_magnitude = (pow( (pow( (double) acc_xyz_data_buffer[0], 2) + pow((double) acc_xyz_data_buffer[1], 2) + pow((double) acc_xyz_data_buffer[2], 2)), 0.5 ));
      acc_magnitude = abs(acc_magnitude - 8192);

      // Append to Acceleration Magnitude Window
      acc_mag_data_window[i] = acc_magnitude;
      if (acc_magnitude > acc_activity_threshold)
      {
        active_sample_count++;
      }
    }

    // Check if inactivity was found while filling data window
    if (active_sample_count < active_sample_count_threshold)
    {
      inactive_window_count++;
    }

    // Begin detection loop
    while (active)
    {
      xDataReadyResult = xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(1000));
      if (xDataReadyResult)
      {
        lsm6dsr_read_single_xl_measurement(p_spi, read_data_buffer);

        // Parse Data Read Buffer
        lsm6dsr_parse_read_data_buffer(read_data_buffer, acc_xyz_data_buffer);

        // Calculate Magnitude
        acc_magnitude = (pow( (pow( (double) acc_xyz_data_buffer[0], 2) + pow((double) acc_xyz_data_buffer[1], 2) + pow((double) acc_xyz_data_buffer[2], 2)), 0.5 ));
        acc_magnitude = abs(acc_magnitude - 8192);

        // Check the first and last data point and modify active sample count
        if (acc_magnitude > acc_activity_threshold)
          active_sample_count++;
        if (acc_mag_data_window[0] < acc_activity_threshold && active_sample_count>0)
          active_sample_count--;

        // Shift all data in buffer to left
        for (int i=0; i < (N-1); i++)
          acc_mag_data_window[i] = acc_mag_data_window[i+1];

        // Set new sample to last element in buffer
        acc_mag_data_window[N-1] = acc_magnitude;
        
        if (active_sample_count < active_sample_count_threshold)
          inactive_window_count++;
        if ((active_sample_count >= active_sample_count_threshold) && (inactive_window_count > 0))
          inactive_window_count--;

        // Check for inactivity
        if (inactive_window_count > inactive_window_count_threshold)
        { 
          // Reset Data Buffers and give Semaphore to Sleep State
          active = false;
          memset(&read_data_buffer, 0, sizeof(read_data_buffer));
          memset(&acc_xyz_data_buffer, 0, sizeof(acc_xyz_data_buffer));
          memset(&acc_mag_data_window, 0, sizeof(acc_mag_data_window));
          active_sample_count = 0;
          inactive_window_count = 0;
          gpio_set_level(LED_BLUE_PIN, 0);
          xSemaphoreGive(*p_xSleepSemaphore);
        }

        // Check for Connected Clients
        xConnectedClientsResult = xSemaphoreTake(*p_xConnectedClientsSemaphore, 0);
        if (xConnectedClientsResult)
          if (*p_connected_clients_count > 0)
          {
            // Reset Data Buffers and give Semaphore to Ready State
            active = false;
            memset(&read_data_buffer, 0, sizeof(read_data_buffer));
            memset(&acc_xyz_data_buffer, 0, sizeof(acc_xyz_data_buffer));
            memset(&acc_mag_data_window, 0, sizeof(acc_mag_data_window));
            active_sample_count = 0;
            inactive_window_count = 0;
            gpio_set_level(LED_BLUE_PIN, 0);
            xSemaphoreGive(*p_xReadySemaphore);
          }
      }
      else
      {
        ESP_LOGI(TAG, "Data Ready Interrupt Timed Out!");
      }
    }
  }
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

  // Create Logging Tag
  static const char *TAG = "Active Task Log";

  // Create Required Variables
  bool ready = false;


  
  for ( ;; )
  {
    // Take Semaphore
    xSemaphoreTake(*p_xReadySemaphore, portMAX_DELAY);
    ESP_LOGI(TAG, "Took Ready Semaphore");
    gpio_set_level(LED_GREEN_PIN, 1);
    ready = true;
  }



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

  // Declare Variables
  bool running = false;
  for ( ;; )
  {
    // Take Semaphore
    xSemaphoreTake(*p_xRunningSemaphore, portMAX_DELAY);
    running = true;
  }
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

  // Declare Variables
  bool networkingActive = false;
  for ( ;; )
  {
    // Take Semaphore
    xSemaphoreTake(*p_xNetworkingActiveSemaphore, portMAX_DELAY);
    networkingActive = true;
  }
}

void networking_inactive_task(void *arg)
{
  // Parse Activity Task Required Resources
  networking_inactive_task_resources_t *p_networking_inactive_task_resources = (networking_inactive_task_resources_t*) arg;

  // State Transition Semaphores
  SemaphoreHandle_t *p_xNetworkingInactiveSemaphore = p_networking_inactive_task_resources->p_xNetworkingInactiveSemaphore;
  SemaphoreHandle_t *p_xNetworkingActiveSemaphore   = p_networking_inactive_task_resources->p_xNetworkingActiveSemaphore;

  // Declare variables
  bool networkingInactive = false;

  for ( ;; )
  {
    // Take Semaphore
    xSemaphoreTake(*p_xNetworkingInactiveSemaphore, portMAX_DELAY);
    networkingInactive = true;
  }

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
  xSemaphoreGive(*p_xSleepSemaphore);
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
  char *p_connected_clients_count = (char*) malloc(sizeof(char));
  *p_connected_clients_count = 0;

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
  xTaskCreatePinnedToCore(sleep_task,   "Sleep Task",   8192,  (void*)p_sleep_task_resources,   1, p_xSleepTaskHandle,   0);
  xTaskCreatePinnedToCore(active_task,  "Active Task",  8192,  (void*)p_active_task_resources,  1, p_xActiveTaskHandle,  0);
  xTaskCreatePinnedToCore(ready_task,   "Ready Task",   8192,  (void*)p_ready_task_resources,   1, p_xReadyTaskHandle,   0);
  xTaskCreatePinnedToCore(running_task, "Running Task", 16384, (void*)p_running_task_resources, 1, p_xRunningTaskHandle, 0);

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
