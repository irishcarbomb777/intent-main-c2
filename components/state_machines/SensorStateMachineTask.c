#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "gpio.h"
#include "SensorStateMachineTask.h"

// Sensor State Machine Defines

void SensorStateMachineTask(void *arg)
{
  // Create Logging Tag
  static const char *TAG = "Sensor State Machine Task Log";

  // Create Sensor State
  SensorStateEnum_t eSensorState = SENSOR_SLEEP;

  // Type cast Sensor State Machine Context from void pointer to Context Pointer
  SensorStateMachineTaskContext_t *p_ctxSensorStateMachineTaskContext = (SensorStateMachineTaskContext_t*) arg;

  // Create Sleep State Context
  SleepStateContext_t ctxSleepState = {
    .p_eSensorState = &eSensorState,
    .p_spi = p_ctxSensorStateMachineTaskContext->p_spi,
    .p_xDataReadySemaphore = p_ctxSensorStateMachineTaskContext->p_xDataReadySemaphore,
    .p_xNetworkInactiveSemaphore = p_ctxSensorStateMachineTaskContext->p_xNetworkInactiveSemaphore,
  };

  // Create Active State Context
  ActiveStateContext_t ctxActiveState = {
    .p_eSensorState = &eSensorState,
    .p_spi = p_ctxSensorStateMachineTaskContext->p_spi,
    .p_xConnectedClientsSemaphore = p_ctxSensorStateMachineTaskContext->p_xConnectedClientsSemaphore,
    .p_xNetworkActiveSemaphore = p_ctxSensorStateMachineTaskContext->p_xNetworkActiveSemaphore,
    .p_cConnectedClientsCount = p_ctxSensorStateMachineTaskContext->p_cConnectedClientsCount,
  };

  // Create Ready State Context
  ReadyStateContext_t ctxReadyState = {
    .p_eSensorState = &eSensorState,
    .p_spi = p_ctxSensorStateMachineTaskContext->p_spi,
    .p_xDataReadySemaphore = p_ctxSensorStateMachineTaskContext->p_xDataReadySemaphore,
    .p_xConnectedClientsSemaphore = p_ctxSensorStateMachineTaskContext->p_xConnectedClientsSemaphore,
    .p_cConnectedClientsCount = p_ctxSensorStateMachineTaskContext->p_cConnectedClientsCount,
  };

  // Create Running State Context
  RunningStateContext_t ctxRunningState = {
    .p_eSensorState = &eSensorState,
    .p_spi = p_ctxSensorStateMachineTaskContext->p_spi,
    .p_xStartSetSemaphore = p_ctxSensorStateMachineTaskContext->p_xStartSetSemaphore,
    .p_xDataTransmitSemaphore = p_ctxSensorStateMachineTaskContext->p_xDataTransmitSemaphore,
    .p_xEndSetSemaphore = p_ctxSensorStateMachineTaskContext->p_xEndSetSemaphore,
    .p_xDataReadySemaphore = p_ctxSensorStateMachineTaskContext->p_xDataReadySemaphore,
    .p_cDataTransmitBuffer = p_ctxSensorStateMachineTaskContext->p_cDataTransmitBuffer,
  };

  // Set lsm6dsr to Initial Sleep State 
  lsm6dsr_sleep_active_state(p_ctxSensorStateMachineTaskContext->p_spi);

  while (1)
  {
    switch (eSensorState)
    {
      case SENSOR_SLEEP:
        vLEDRedState();
        vSleepState(&ctxSleepState);
        break;
      case SENSOR_ACTIVE:
        vLEDBlueState();
        vActiveState(&ctxActiveState);
        break;
      case SENSOR_READY:
        vLEDPurpleState();
        vReadyState(&ctxReadyState);
        break;
      case SENSOR_RUNNING:
        vLEDGreenState();
        vRunningState(&ctxRunningState);
        break;
      default:
        break;
    }
  }
}

// Sensor State Machine Utility Functions
// ANY FUNCTIONS USED BY MULTIPLE STATES ARE HERE
void lsm6dsr_sleep_active_state(spi_device_handle_t *p_spi)
{
  // Turn off all interrupts
  lsm6dsr_write_register(p_spi, INT1_CTRL, 0x00);

  // Turn off XL
  lsm6dsr_write_register(p_spi, CTRL1_XL, 0x08);

  // Turn off Gyroscope
  lsm6dsr_write_register(p_spi, CTRL2_G, 0x04);

  // Set to low power
  lsm6dsr_write_register(p_spi, CTRL6_C, 0x10);

  // Set Data Ready Interrupt
  lsm6dsr_write_register(p_spi, INT1_CTRL, 0x01);

  // Set ODR to 52Hz
  lsm6dsr_write_register(p_spi, CTRL1_XL, 0x38);
}

void read_write_magnitude_to_buffer(spi_device_handle_t *p_spi, double *p_dBuffer, unsigned int *p_uintIndex)
{
  // Create Static Data Buffers
  static char data_read_buffer[7];
  static int16_t xyz_data_buffer[3];
  static double magnitude;

  // Read Bytes and Calculate Magnitude
  lsm6dsr_read_single_xl_measurement(p_spi, data_read_buffer);
  lsm6dsr_parse_read_data_buffer(data_read_buffer, xyz_data_buffer);
  magnitude = (pow( (pow( (double) xyz_data_buffer[0], 2) + pow((double) xyz_data_buffer[1], 2) + pow((double) xyz_data_buffer[2], 2)), 0.5 ));

  // Place Magnitude in Buffer
  *(p_dBuffer+(*p_uintIndex)) = magnitude;
}

uint circular_add(int x, uint val, uint buffer_length)
{
  // Create Logging Tag
  static const char *TAG = "Circular Add Log";
  if (is_power_of_two(buffer_length))
  {
    uint mask = ~buffer_length;
    val = (((val | mask) + x) & buffer_length);
    return val;
  }
  else
  {
    ESP_LOGI(TAG, "Buffer Length Specified is not a power of 2");
    return 0;
  }
}

uint circular_subtract(int x, uint val, uint buffer_length)
{
  // Create Logging Tag
  static const char *TAG = "Circular Subtract Log";
  if (is_power_of_two(buffer_length))
  {
    uint mask = ~buffer_length;
    val = (((val | mask) - x) & buffer_length);
    return val;
  }
  else
  {
    ESP_LOGI(TAG, "Buffer Length Specified is not a power of 2");
    return 0;
  }
}

bool is_power_of_two(uint buffer_length)
{
    if (buffer_length == 0)
        return 0;
    while (buffer_length != 1) {
        if (buffer_length % 2 != 0)
            return 0;
        buffer_length = buffer_length / 2;
    }
    return 1;
}