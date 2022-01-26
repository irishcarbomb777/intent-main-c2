#pragma once
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "rtos_tasks.h"

// Function Prototypes
void initialize_gpio_w_isr(gpio_config_t *cfg);
void gpio_startup_routine(task_handles_t *task_handles);