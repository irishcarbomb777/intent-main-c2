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

/********************************************************
************ Startup Routine Code ***********************
********************************************************/

// Set define constants
#define INT1_PIN 21
#define INT2_PIN 22

#define INT_PIN_MASK (1ULL<<INT1_PIN | 1ULL<<INT2_PIN)

static void gpio_activity_inactivity_isr_handler(void *arg);
static void gpio_fifo_watermark_isr_handler(void *arg);

void gpio_startup_routine(task_handles_t *task_handles)
{
  // Parse out task handles
  SemaphoreHandle_t *p_xActivityToggleSemaphore   = task_handles->p_xActivityToggleSemaphore;
  SemaphoreHandle_t *p_xFifoReadySemaphore = task_handles->p_xFifoReadySemaphore;

  // Initialize activity/inactivity interrupt
  gpio_config_t gpio_activity_inactivity_config = {
    .intr_type = GPIO_INTR_POSEDGE,
    .mode = GPIO_MODE_INPUT,
    .pin_bit_mask = INT_PIN_MASK,
    .pull_down_en = 1,
    .pull_up_en = 0,
  };
  initialize_gpio_w_isr(&gpio_activity_inactivity_config);
  gpio_isr_handler_add( INT1_PIN, gpio_activity_inactivity_isr_handler, (void*)p_xActivityToggleSemaphore);
  gpio_isr_handler_add( INT2_PIN, gpio_fifo_watermark_isr_handler, (void*)p_xFifoReadySemaphore);
}


static void gpio_activity_inactivity_isr_handler(void *arg)
{
  // ESP_LOGI("MAIN_TAfdsafG", " Alarm has been raised ");
  // Create force scheduler variable
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  // Parse Activity Task Handle out of Void Pointer
  SemaphoreHandle_t xActivityToggleSemaphore = *(SemaphoreHandle_t*) arg;
  
  // Send Notification to Activity Task
  xSemaphoreGiveFromISR(xActivityToggleSemaphore, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

static void gpio_fifo_watermark_isr_handler(void *arg)
{
  // ESP_LOGI("MAIN_TAfdsafG", " Alarm has been raised ");
  // Create force scheduler variable
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  // Parse activity task handle from arg
  SemaphoreHandle_t xFifoReadySemaphore = *(SemaphoreHandle_t*) arg;

  // Send Notification to Activity Task
  xSemaphoreGiveFromISR( xFifoReadySemaphore, &xHigherPriorityTaskWoken );
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}



