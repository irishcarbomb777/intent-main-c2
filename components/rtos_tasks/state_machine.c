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
#include "gpio.h"
#include "state_machine.h"
#include "lsm6dsr.h"
#include "wifi.h"

#define low_ODR_N 52
#define mid_ODR_N 208
#define high_ODR_N 6660
#define watermark_threshold 300

void sleep_state(sleep_state_resources_t *p_sleep_state_resources, sensor_state_enum_t *p_sensor_state)
{
  // Communication Protocols
  spi_device_handle_t *p_spi = p_sleep_state_resources->p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore = p_sleep_state_resources->p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xNetworkInactiveSemaphore = p_sleep_state_resources->p_xNetworkInactiveSemaphore;

  // Create Logging Tag
  static const char *TAG = "Sleep Task Log";
  
  // Create Static Data Variables
  static BaseType_t xDataReadyResult;
  static char data_read_buffer[7];
  static int16_t acc_xyz_data_buffer[3];
  static double acc_magnitude;
  static double acc_magnitude_offset = 8234;
  static double acc_mag_data_window[low_ODR_N];
  static char active_sample_count = 0;
  static char active_count_threshold = 2;
  static double acc_activity_threshold = 100;

  // Set LED Color
  gpio_set_level(LED_RED_PIN, 1);

  // Give Network Inactive Semaphore
  xSemaphoreGive(*p_xNetworkInactiveSemaphore);

  // Set lsm6dsr to low power mode
  lsm6dsr_enter_sleep_active_state(p_spi);

  // Fill the Acceleration Magnitude Data Window
  for ( int i=0; i < low_ODR_N; i++ )
  {
    // Wait for Data to be ready
    xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(1000));
    // Read Data
    lsm6dsr_read_single_xl_measurement(p_spi, data_read_buffer);
    // Parse Data Read Buffer
    lsm6dsr_parse_read_data_buffer(data_read_buffer, acc_xyz_data_buffer);
    // Calculate Magnitude
    acc_magnitude = (pow( (pow( (double) acc_xyz_data_buffer[0], 2) + pow((double) acc_xyz_data_buffer[1], 2) + pow((double) acc_xyz_data_buffer[2], 2)), 0.5 ));
    acc_magnitude = abs(acc_magnitude - acc_magnitude_offset);
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
    *p_sensor_state = SENSOR_ACTIVE;
    memset(&data_read_buffer, 0, sizeof(data_read_buffer));
    memset(&acc_xyz_data_buffer, 0, sizeof(acc_xyz_data_buffer));
    memset(&acc_mag_data_window, 0, sizeof(acc_mag_data_window));
    active_sample_count = 0;
    gpio_set_level(LED_RED_PIN, 0);
  }

  // Begin Detection Loop
  while ( (*p_sensor_state) == SENSOR_SLEEP )
  {
    xDataReadyResult = xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(1000));
    if (xDataReadyResult)
    {
      lsm6dsr_read_single_xl_measurement(p_spi, data_read_buffer);

      // Parse Data Read Buffer
      lsm6dsr_parse_read_data_buffer(data_read_buffer, acc_xyz_data_buffer);

      // Calculate Magnitude
      acc_magnitude = (pow( (pow( (double) acc_xyz_data_buffer[0], 2) + pow((double) acc_xyz_data_buffer[1], 2) + pow((double) acc_xyz_data_buffer[2], 2)), 0.5 ));
      acc_magnitude = abs(acc_magnitude - acc_magnitude_offset);
      // Check the first and last data point and modify active sample count
      if (acc_magnitude > acc_activity_threshold)
      {
        active_sample_count++;
      }
      if (acc_mag_data_window[0] < acc_activity_threshold && (active_sample_count > 0))
        active_sample_count--;

      // Shift all data in buffer to left
      for (int i=0; i < (low_ODR_N-1); i++)
        acc_mag_data_window[i] = acc_mag_data_window[i+1];

      // Set new sample to last element in buffer
      acc_mag_data_window[low_ODR_N-1] = acc_magnitude;
      
      if (active_sample_count > active_count_threshold)
      {
        // Reset Data Buffers and give Semaphore to Active State
        *p_sensor_state = SENSOR_ACTIVE; 
        memset(&data_read_buffer, 0, sizeof(data_read_buffer));
        memset(&acc_xyz_data_buffer, 0, sizeof(acc_xyz_data_buffer));
        memset(&acc_mag_data_window, 0, sizeof(acc_mag_data_window));
        active_sample_count = 0;
        gpio_set_level(LED_RED_PIN, 0);
      }
    }
    else
    {
      lsm6dsr_read_single_xl_measurement(p_spi, data_read_buffer);
      ESP_LOGI(TAG, "Interrupt Timed out(Line 150)");
    }
  }
}

void active_state(active_state_resources_t *p_active_state_resources, sensor_state_enum_t *p_sensor_state)
{
  // Communication Protocols
  spi_device_handle_t *p_spi = p_active_state_resources->p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore = p_active_state_resources->p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xNetworkActiveSemaphore = p_active_state_resources->p_xNetworkActiveSemaphore;
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
  static double acc_magnitude_offset = 8234;
  static double acc_mag_data_window[low_ODR_N];
  static char active_sample_count = 0;
  static double acc_activity_threshold = 100;
  static char active_sample_count_threshold = 2;
  static unsigned int inactive_window_count = 0;
  static unsigned int inactive_window_count_threshold = low_ODR_N*15; // Wait for 4 minutes

  // Set LED Color
  gpio_set_level(LED_BLUE_PIN, 1);

  // Give Network Active Semaphore
  xSemaphoreGive(*p_xNetworkActiveSemaphore);

  // Set lsm6dsr to low power mode
  lsm6dsr_enter_sleep_active_state(p_spi);

  // Fill the Acceleration Magnitude Data Window
  for ( int i=0; i < low_ODR_N; i++ )
  {
    // Wait for Data to be ready
    xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(1000));

    // Read Data
    lsm6dsr_read_single_xl_measurement(p_spi, read_data_buffer);

    // Parse Data Read Buffer
    lsm6dsr_parse_read_data_buffer(read_data_buffer, acc_xyz_data_buffer);

    // Calculate Magnitude
    acc_magnitude = (pow( (pow( (double) acc_xyz_data_buffer[0], 2) + pow((double) acc_xyz_data_buffer[1], 2) + pow((double) acc_xyz_data_buffer[2], 2)), 0.5 ));
    acc_magnitude = abs(acc_magnitude - acc_magnitude_offset);

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

  while ((*p_sensor_state) == SENSOR_ACTIVE)
  {
    xDataReadyResult = xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(1000));
    if (xDataReadyResult)
    {
      lsm6dsr_read_single_xl_measurement(p_spi, read_data_buffer);

      // Parse Data Read Buffer
      lsm6dsr_parse_read_data_buffer(read_data_buffer, acc_xyz_data_buffer);

      // Calculate Magnitude
      acc_magnitude = (pow( (pow( (double) acc_xyz_data_buffer[0], 2) + pow((double) acc_xyz_data_buffer[1], 2) + pow((double) acc_xyz_data_buffer[2], 2)), 0.5 ));
      acc_magnitude = abs(acc_magnitude - acc_magnitude_offset);

      // Check the first and last data point and modify active sample count
      if (acc_magnitude > acc_activity_threshold)
        active_sample_count++;
      if (acc_mag_data_window[0] < acc_activity_threshold && active_sample_count>0)
        active_sample_count--;

      // Shift all data in buffer to left
      for (int i=0; i < (low_ODR_N-1); i++)
        acc_mag_data_window[i] = acc_mag_data_window[i+1];

      // Set new sample to last element in buffer
      acc_mag_data_window[low_ODR_N-1] = acc_magnitude;
      
      if (active_sample_count < active_sample_count_threshold)
        inactive_window_count++;
      if ((active_sample_count >= active_sample_count_threshold) && (inactive_window_count > 0))
        inactive_window_count--;

      // Check for inactivity
      if (inactive_window_count > inactive_window_count_threshold)
      { 
        // Reset Data Buffers and give Semaphore to Sleep State
        *p_sensor_state = SENSOR_SLEEP;
        memset(&read_data_buffer, 0, sizeof(read_data_buffer));
        memset(&acc_xyz_data_buffer, 0, sizeof(acc_xyz_data_buffer));
        memset(&acc_mag_data_window, 0, sizeof(acc_mag_data_window));
        active_sample_count = 0;
        inactive_window_count = 0;
        gpio_set_level(LED_BLUE_PIN, 0);
      }

      // Check for Connected Clients
      xConnectedClientsResult = xSemaphoreTake(*p_xConnectedClientsSemaphore, 0);
      if (xConnectedClientsResult)
        if (*p_connected_clients_count > 0)
        {
          // Reset Data Buffers and give Semaphore to Ready State
          *p_sensor_state = SENSOR_READY;
          memset(&read_data_buffer, 0, sizeof(read_data_buffer));
          memset(&acc_xyz_data_buffer, 0, sizeof(acc_xyz_data_buffer));
          memset(&acc_mag_data_window, 0, sizeof(acc_mag_data_window));
          active_sample_count = 0;
          inactive_window_count = 0;
          gpio_set_level(LED_BLUE_PIN, 0);
        }
    }
    else
    {
      ESP_LOGI(TAG, "Data Ready Interrupt Timed Out!");
      lsm6dsr_read_single_xl_measurement(p_spi, read_data_buffer);
    }
  }
}

void ready_state(ready_state_resources_t *p_ready_state_resources, sensor_state_enum_t *p_sensor_state)
{
  // Communication Protocols
  spi_device_handle_t *p_spi = p_ready_state_resources->p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore        = p_ready_state_resources->p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xConnectedClientsSemaphore = p_ready_state_resources->p_xConnectedClientsSemaphore;

  // Shared Data Buffers
  char *p_connected_clients_count = p_ready_state_resources->p_connected_clients_count;

  // Create Static Data Variables
  static const char *TAG = "Ready Task Log";
  static BaseType_t xDataReadyResult;
  static BaseType_t xConnectedClientsResult;
  static char data_read_buffer[7];
  static int16_t acc_xyz_data_buffer[3];
  static double acc_magnitude = 0;
  static double acc_magnitude_offset = 8234;
  static double vel_mag_data_window[mid_ODR_N];
  static double position_integral = 0;
  static double position_threshold = 0.02;
  static double dt = ((double)1/(double)mid_ODR_N);
  vel_mag_data_window[0] = 0;
  // Set LED Color
  gpio_set_level(LED_BLUE_PIN, 1);
  gpio_set_level(LED_RED_PIN, 1);

  // Set lsm6dsr to ready state mode
  lsm6dsr_enter_ready_state(p_spi);
  // Wait for Data to be Ready and Clear out Junk Read
  // Clear out Junk Data
  for (int i=0; i < 20; i++)
  {
    xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(1000));
    lsm6dsr_read_single_xl_measurement(p_spi, data_read_buffer);
    lsm6dsr_parse_read_data_buffer(data_read_buffer, acc_xyz_data_buffer);
  }

  // Fill the Velocity Magnitude Data Window
  for (int i=1; i < mid_ODR_N; i++)
  {
    // Wait for Data to be Ready
    xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(100));
    // Read Data
    lsm6dsr_read_single_xl_measurement(p_spi, data_read_buffer);
    // Parse Data Read Buffer
    lsm6dsr_parse_read_data_buffer(data_read_buffer, acc_xyz_data_buffer);
    // Calculate Magnitude
    acc_magnitude = (pow( (pow( (double) acc_xyz_data_buffer[0], 2) + pow((double) acc_xyz_data_buffer[1], 2) + pow((double) acc_xyz_data_buffer[2], 2)), 0.5 ));
    acc_magnitude = acc_magnitude - acc_magnitude_offset;
    // ESP_LOGI(TAG, "line 339 magnitude is %.2f", acc_magnitude);
    if (acc_magnitude > 100)
    {
      vel_mag_data_window[i] = (acc_magnitude/acc_magnitude_offset)*9.81*dt + vel_mag_data_window[i-1];
      position_integral += (vel_mag_data_window[i]*dt);
    }
  }

  // Check if Significant Motion was found in fill stage
  if (position_integral > position_threshold)
  {
    *p_sensor_state = SENSOR_RUNNING;
    memset(&data_read_buffer, 0, sizeof(data_read_buffer));
    memset(&acc_xyz_data_buffer, 0, sizeof(acc_xyz_data_buffer));
    memset(&vel_mag_data_window, 0, sizeof(vel_mag_data_window));
    position_integral = 0;
    gpio_set_level(LED_BLUE_PIN, 0);
    gpio_set_level(LED_RED_PIN, 0);
  }

  // Begin Detection Loop
  while ( (*p_sensor_state)==SENSOR_READY )
  {
    xDataReadyResult = xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(100));
    if (xDataReadyResult)
    {
      lsm6dsr_read_single_xl_measurement(p_spi, data_read_buffer);

      // Parse Data Read Buffer
      lsm6dsr_parse_read_data_buffer(data_read_buffer, acc_xyz_data_buffer);

      // Calculate Magnitude
      acc_magnitude = (pow( (pow( (double) acc_xyz_data_buffer[0], 2) + pow((double) acc_xyz_data_buffer[1], 2) + pow((double) acc_xyz_data_buffer[2], 2)), 0.5 ));
      acc_magnitude = acc_magnitude - acc_magnitude_offset;
      if (acc_magnitude > 100)
      {
        // ESP_LOGI(TAG, "line 374 magnitude is %.2f", acc_magnitude);
        // Subtract least recent sample from the position integral
        position_integral -= vel_mag_data_window[0]*dt;
        
        // Shift all data in buffer to left
        for (int i=0; i < (mid_ODR_N-1); i++)
          vel_mag_data_window[i] = vel_mag_data_window[i+1];
        
        // Set new sample to last element in buffer
        vel_mag_data_window[mid_ODR_N-1] = (acc_magnitude/acc_magnitude_offset)*9.81*dt + vel_mag_data_window[mid_ODR_N-2];
        // ESP_LOGI(TAG, "velocity is %.2f", vel_mag_data_window[mid_ODR_N-1]);
        position_integral += vel_mag_data_window[mid_ODR_N-1]*dt;
        // ESP_LOGI(TAG, "Position Integral is: %.2f", position_integral);
      }


      // Check for 2 Inches of movement
      if (position_integral > position_threshold)
      {
        // ESP_LOGI(TAG, "LINE 382");
        *p_sensor_state = SENSOR_RUNNING;
        memset(&data_read_buffer, 0, sizeof(data_read_buffer));
        memset(&acc_xyz_data_buffer, 0, sizeof(acc_xyz_data_buffer));
        memset(&vel_mag_data_window, 0, sizeof(vel_mag_data_window));
        position_integral = 0;
        acc_magnitude = 0;
        gpio_set_level(LED_BLUE_PIN, 0);
        gpio_set_level(LED_RED_PIN, 0);
      }
      // Check for Connected Clients
      xConnectedClientsResult = xSemaphoreTake(*p_xConnectedClientsSemaphore, 0);
      if (xConnectedClientsResult)
        if (*p_connected_clients_count < 1)
        {
          // Reset Data Buffers and give Semaphore to Ready State
          *p_sensor_state = SENSOR_ACTIVE;
          memset(&data_read_buffer, 0, sizeof(data_read_buffer));
          memset(&acc_xyz_data_buffer, 0, sizeof(acc_xyz_data_buffer));
          memset(&vel_mag_data_window, 0, sizeof(vel_mag_data_window));
          position_integral = 0;
          acc_magnitude = 0;
          gpio_set_level(LED_BLUE_PIN, 0);
          gpio_set_level(LED_RED_PIN, 0);
        }
    }
    else
    {
      lsm6dsr_read_single_xl_measurement(p_spi, data_read_buffer);
      ESP_LOGI(TAG, "Interrupt Timed out(Line 150)");
    } 
  }
}


void running_state(running_state_resources_t *p_running_state_resources, sensor_state_enum_t *p_sensor_state)
{
  // Communication Protocols
  spi_device_handle_t *p_spi = p_running_state_resources->p_spi;

  // Signal Semaphores
  SemaphoreHandle_t *p_xDataReadySemaphore    = p_running_state_resources->p_xDataReadySemaphore;
  SemaphoreHandle_t *p_xDataTransmitSemaphore = p_running_state_resources->p_xDataTransmitSemaphore;
  SemaphoreHandle_t *p_xStartSetSemaphore     = p_running_state_resources->p_xStartSetSemaphore;
  SemaphoreHandle_t *p_xEndSetSemaphore       = p_running_state_resources->p_xEndSetSemaphore;

  // Shared Data Buffers
  char *p_data_transmit_buffer = p_running_state_resources->p_data_transmit_buffer;

  // Create Static Data Variables
  static const char *TAG = "Running Task Log";
  static BaseType_t xDataReadyResult;
  static BaseType_t xDataTransmitResult;
  static char p_fifo_data_buffer[(7*watermark_threshold)+1];
  static double acc_magnitude;
  static double acc_magnitude_offset = 8234;
  static double p_acc_range_buffer[1600];
  p_acc_range_buffer[0] = 0;
  static int p_acc_range_buffer_memory_pointer = 0;
  static int inactivity_count = 0;
  static int interval_count = 0;
  static int16_t acc_xyz_data_buffer[3];

  // Set LED Color
  gpio_set_level(LED_GREEN_PIN, 1);

  // Set lsm6dsr to high power mode
  lsm6dsr_enter_running_state(p_spi);
  while ((*p_sensor_state) == SENSOR_RUNNING)
  { 
    xDataReadyResult = xSemaphoreTake(*p_xDataReadySemaphore, pdMS_TO_TICKS(1000));
    if (xDataReadyResult)
    {
      lsm6dsr_batch_read_fifo(p_spi, watermark_threshold, p_fifo_data_buffer);
      memcpy(p_data_transmit_buffer, p_fifo_data_buffer, sizeof(p_fifo_data_buffer));
      *(p_data_transmit_buffer+ ((watermark_threshold*7)+1 )) = '\0';
      xSemaphoreGive(*p_xDataTransmitSemaphore);
      for (int i=1; i < 2096; i = i+7)
      {
        interval_count++;
        if ( ((p_fifo_data_buffer[i]>>3) == 2) && (interval_count > 75))
        {
          interval_count = 0;
          if (p_acc_range_buffer[p_acc_range_buffer_memory_pointer] < 500 && (p_acc_range_buffer[p_acc_range_buffer_memory_pointer]!=0))
            inactivity_count++;
          if (p_acc_range_buffer[p_acc_range_buffer_memory_pointer] >= 500 && (inactivity_count>0))
            inactivity_count--;
          lsm6dsr_parse_read_data_buffer((p_fifo_data_buffer+i), acc_xyz_data_buffer);
          acc_magnitude = (pow( (pow( (double) acc_xyz_data_buffer[0], 2) + pow((double) acc_xyz_data_buffer[1], 2) + pow((double) acc_xyz_data_buffer[2], 2)), 0.5 ));
          p_acc_range_buffer[p_acc_range_buffer_memory_pointer+1] = (acc_magnitude-acc_magnitude_offset);
          p_acc_range_buffer_memory_pointer++;
          if (p_acc_range_buffer_memory_pointer > 1597)
            p_acc_range_buffer_memory_pointer = 0;
        }
      }
      if (inactivity_count > 1500)
      {
        *p_sensor_state = SENSOR_READY;
        memset(&p_fifo_data_buffer, 0, sizeof(p_fifo_data_buffer));
        memset(&p_acc_range_buffer, 0, sizeof(p_acc_range_buffer));
        memset(&acc_xyz_data_buffer, 0, sizeof(acc_xyz_data_buffer));
        acc_magnitude = 0;
        inactivity_count = 0;
        interval_count = 0;
        p_acc_range_buffer_memory_pointer = 0;
        gpio_set_level(LED_GREEN_PIN, 0);
      }
    }
    else
    {
      lsm6dsr_batch_read_fifo(p_spi, watermark_threshold, p_fifo_data_buffer);
      ESP_LOGI(TAG, "FIFO Timeout");
    }
  }
}


void network_active_state(network_active_state_resources_t *p_networking_active_state_resources, network_state_enum_t *p_network_state)
{
  // Communication Protocols
  esp_mqtt_client_handle_t *p_mqtt_client = p_networking_active_state_resources->p_mqtt_client;

  // Signal Semaphores
  SemaphoreHandle_t *p_xConnectedClientsSemaphore = p_networking_active_state_resources->p_xConnectedClientsSemaphore;
  SemaphoreHandle_t *p_xStartSetSemaphore = p_networking_active_state_resources->p_xStartSetSemaphore;
  SemaphoreHandle_t *p_xDataTransmitSemaphore = p_networking_active_state_resources->p_xDataTransmitSemaphore;
  SemaphoreHandle_t *p_xEndSetSemaphore = p_networking_active_state_resources->p_xEndSetSemaphore;
  SemaphoreHandle_t *p_xNetworkInactiveSemaphore = p_networking_active_state_resources->p_xNetworkInactiveSemaphore;

  // Shared Data Buffers
  char *p_connected_clients_count = p_networking_active_state_resources->p_connected_clients_count;
  char *p_data_transmit_buffer = p_networking_active_state_resources->p_data_transmit_buffer;

  // Initialize Static Variables
  static BaseType_t xDataTransmitResult;
  static BaseType_t xNetworkInactiveResult;

  // Enter high power mode
  wifi_disable_modem_sleep();

  // Publish to Active Sensors Channel
  esp_mqtt_client_publish(*p_mqtt_client, "active_sensors", "sensor_01", 0, 0, 0);

  // Subscribe to Client Connections Channel
  esp_mqtt_client_subscribe(*p_mqtt_client, "client_connections/sensor_01", 1);

  while ((*p_network_state) == NETWORK_ACTIVE)
  {
    xDataTransmitResult = xSemaphoreTake(*p_xDataTransmitSemaphore, pdMS_TO_TICKS(1000));
    if (xDataTransmitResult)
      esp_mqtt_client_publish(*p_mqtt_client, "data/sensor_01", p_data_transmit_buffer, (300*7)+2, 0, 0 );
    xNetworkInactiveResult = xSemaphoreTake(*p_xNetworkInactiveSemaphore, 0);
    if (xNetworkInactiveResult)
      *p_network_state = NETWORK_INACTIVE;
  }
}



void network_inactive_state(network_inactive_state_resources_t *p_network_inactive_state_resources, network_state_enum_t *p_network_state)
{
  // Communication Protocols
  esp_mqtt_client_handle_t *p_mqtt_client = p_network_inactive_state_resources->p_mqtt_client;
  // Signal Semaphores
  SemaphoreHandle_t *p_xNetworkActiveSemaphore = p_network_inactive_state_resources->p_xNetworkActiveSemaphore;

  // Unsubscribe from MQTT Channels
  esp_mqtt_client_unsubscribe(*p_mqtt_client, "client_connections");

  // Set Networking to Low Power Mode
  wifi_enable_modem_sleep();

  // Create Local Variables
  static BaseType_t xNetworkActiveResult;

  while ((*p_network_state) == NETWORK_INACTIVE)
  {
    xNetworkActiveResult = xSemaphoreTake(*p_xNetworkActiveSemaphore, pdMS_TO_TICKS(1000));
    if (xNetworkActiveResult)
      *p_network_state = NETWORK_ACTIVE;
  }


}