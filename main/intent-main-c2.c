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
#include "rtos_tasks.h"
#include "gpio.h"

void app_main(void)
{
  spi_device_handle_t *p_spi = lsm6dsr_startup_routine(); 
  gpio_initialize_output_led();
  rtos_tasks_shared_resources_t *p_rtos_task_resources = rtos_tasks_startup_routine(p_spi);
  gpio_startup_routine(p_rtos_task_resources);

  while(1)
  {
    vTaskDelay(portMAX_DELAY);
  }
}
