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

#include "lsm6dsr.h"
// #include "rtos_tasks.h"
// #include "gpio.h"


void app_main(void)
{
  // Set define constants
  #define PIN_MISO       12
  #define PIN_MOSI       13
  #define PIN_CLK        14
  #define PIN_CS         27
    // Configure SPI for lsm6dsr
  spi_bus_config_t buscfg = {
    .miso_io_num   = PIN_MISO,
    .mosi_io_num   = PIN_MOSI,
    .sclk_io_num   = PIN_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1
  };
  spi_device_interface_config_t devcfg={
    .command_bits     = 0,
    .address_bits     = 0,
    .dummy_bits       = 0,
    .clock_speed_hz   = 1E6,
    .duty_cycle_pos   = 128,
    .mode             = 3,
    .spics_io_num     = PIN_CS,
    .cs_ena_posttrans = 3,
    .queue_size       = 3
  };
  spi_device_handle_t *p_lsm6dsr_spi_handle = lsm6dsr_initialize_spi(&buscfg, &devcfg);
  char register_value; 
  for (int i=0; i < 20; i++)
  {
    register_value = lsm6dsr_read_register(p_lsm6dsr_spi_handle, WHO_AM_I);
  }
  while(1)
  {
    vTaskDelay(portMAX_DELAY);
  }
}
