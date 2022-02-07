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

#include "gpio.h"
#include "state_machine.h"

typedef enum {
  SLEEP = 0,
  ACTIVE = 1,
  READY = 2,
  RUNNING = 3,
} sensor_state_enum_t;

void state_machine_task(void *arg)
{
  // Create Required Variables
  sensor_state_enum_t state = SLEEP;

  while (1)
  {
    switch (state)
    {
      case SLEEP:
        sleep_state(); 
        break;
      case ACTIVE:
        active_state();
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


void sleep_state(sleep_state_resources_t *p_sleep_state_resources, sensor_state_enum_t *p_sensor_state)
{
  // Parse Sleep State Required Resources
  sleep_state_resources_t *p_sleep_state_resources = (sleep_state_resources_t*) arg;

  // Communication Protocols
  spi_device_handle_t *p_spi = p_sleep_state_resources->p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore = p_sleep_state_resources->p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xNetworkingInactiveSemaphore = p_sleep_state_resources->p_xNetworkingInactiveSemaphore;

  // Create Static Data Variables
  BaseType_t xDataReadyResult;
  static char data_read_buffer[7];
  static int16_t acc_xyz_data_buffer[3];
  static double acc_magnitude;
  static const char N = 52;
  static double acc_mag_data_window[N];
  static char active_sample_count = 0;
  static char active_count_threshold = 2;
  static double acc_activity_threshold = 100;

  /* Run LED Task */
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
    *p_sensor_state = ACTIVE;
    memset(&data_read_buffer, 0, sizeof(data_read_buffer));
    memset(&acc_xyz_data_buffer, 0, sizeof(acc_xyz_data_buffer));
    memset(&acc_mag_data_window, 0, sizeof(acc_mag_data_window));
    active_sample_count = 0;
    gpio_set_level(LED_RED_PIN, 0);
  }

  // Begin Detection Loop
  while ( (*p_sensor_state) == SLEEP )
  {
    xDataReadyResult = xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(1000));
    if (xDataReadyResult)
    {
      lsm6dsr_read_single_xl_measurement(p_spi, data_read_buffer);

      // Parse Data Read Buffer
      lsm6dsr_parse_read_data_buffer(data_read_buffer, acc_xyz_data_buffer);

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
        *p_sensor_state = ACTIVE; 
        memset(&data_read_buffer, 0, sizeof(data_read_buffer));
        memset(&acc_xyz_data_buffer, 0, sizeof(acc_xyz_data_buffer));
        memset(&acc_mag_data_window, 0, sizeof(acc_mag_data_window));
        active_sample_count = 0;
        gpio_set_level(LED_RED_PIN, 0);
      }
    }
    else
    {
      lsm6dsr_read_single_xl_measurement(p_spi, read_data_buffer);
      ESP_LOGI(TAG, "Interrupt Timed out(Line 150)");
    }
  }
}

void active_state(active_state_resources_t *p_active_state_resources, sensor_state_enum_t *p_sensor_state)
{
  // Parse Active State Required Resources
  active_state_resources_t *p_active_state_resources = (active_state_resources_t*) arg;

  // Communication Protocols
  spi_device_handle_t *p_spi = p_active_state_resources->p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore = p_active_state_resources->p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xNetworkingActiveSemaphore = p_active_state_resources->p_xNetworkingInactiveSemaphore;
  SemaphoreHandle_t *p_xConnectedClientsSemaphore = p_active_state_resources-> p_xConnectedClientsSemaphore;

  // Shared Data Buffers
  char *p_connected_clients_count = p_active_state_resources->p_connected_clients_count;

  // Create Logging Tag
  static const char *TAG = "Active Task Log";
  static BaseType_t xDataReadyResult;
  static BaseType_t xConnectedClientsResult;
  static char read_data_buffer[7];
  static int16_t acc_xyz_data_buffer[3];
  static double acc_magnitude;
  static const char N = 52;
  static double acc_mag_data_window[N];
  static char active_sample_count = 0;
  static double acc_activity_threshold = 100;
  static char active_sample_count_threshold = 2;
  static unsigned int inactive_window_count = 0;
  static unsigned int inactive_window_count_threshold = N*5; // Wait for 4 minutes

  // Run LED Task
  
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

  while ((*p_sensor_state) == ACTIVE)
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
        *p_sensor_state = SLEEP;
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
          *p_sensor_state = READY;
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