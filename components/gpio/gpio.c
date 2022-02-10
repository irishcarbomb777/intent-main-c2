#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"

#include "sdkconfig.h"

#include "gpio.h"
#include "rtos_tasks.h"

void initialize_gpio_w_isr(gpio_config_t *cfg)
{
  // Set logging tag 
  gpio_config(cfg);
  gpio_install_isr_service(ESP_INTR_FLAG_EDGE); 
}

void gpio_initialize_output_led()
{
  gpio_config_t gpio_led_config = {
    .intr_type = GPIO_INTR_DISABLE,
    .mode = GPIO_MODE_OUTPUT,
    .pin_bit_mask = LED_OUTPUT_MASK,
    .pull_down_en = 0,
    .pull_up_en = 0,
  };
  gpio_config(&gpio_led_config);
}

void vLEDRedState()
{
  gpio_set_level(LED_RED_PIN, 1);
  gpio_set_level(LED_BLUE_PIN, 0);
  gpio_set_level(LED_GREEN_PIN, 0);
}

void vLEDGreenState()
{
  gpio_set_level(LED_RED_PIN, 0);
  gpio_set_level(LED_BLUE_PIN, 0);
  gpio_set_level(LED_GREEN_PIN, 1);
}

void vLEDBlueState()
{
  gpio_set_level(LED_RED_PIN, 0);
  gpio_set_level(LED_BLUE_PIN, 1);
  gpio_set_level(LED_GREEN_PIN, 0);
}

void vLEDPurpleState()
{
  gpio_set_level(LED_RED_PIN, 1);
  gpio_set_level(LED_BLUE_PIN, 1);
  gpio_set_level(LED_GREEN_PIN, 0);
}

/********************************************************
************ Startup Routine Code ***********************
********************************************************/

// Set define constants
#define INT1_PIN 21
#define INT_PIN_MASK (1ULL<<INT1_PIN)

static void gpio_data_ready_isr_handler(void *arg);

void gpio_startup_routine(rtos_tasks_shared_resources_t *rtos_task_resources)
{
  // Parse out task handles
  SemaphoreHandle_t *p_xDataReadySemaphore   = rtos_task_resources->p_xDataReadySemaphore;

  // Initialize Data Ready interrupt
  gpio_config_t gpio_activity_inactivity_config = {
    .intr_type = GPIO_INTR_POSEDGE,
    .mode = GPIO_MODE_INPUT,
    .pin_bit_mask = INT_PIN_MASK,
    .pull_down_en = 0,
    .pull_up_en = 0,
  };
  initialize_gpio_w_isr(&gpio_activity_inactivity_config);
  gpio_isr_handler_add( INT1_PIN, gpio_data_ready_isr_handler, (void*)p_xDataReadySemaphore);

}

static void gpio_data_ready_isr_handler(void *arg)
{
  // Create force scheduler variable
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  // Parse Activity Task Handle out of Void Pointer
  SemaphoreHandle_t xDataReadySemaphore = *(SemaphoreHandle_t*) arg;
  
  // Send Notification to Activity Task
  xSemaphoreGiveFromISR(xDataReadySemaphore, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}




