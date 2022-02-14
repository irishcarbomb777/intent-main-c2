#pragma once
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// #include "rtos_tasks.h"

#define LED_BLUE_PIN 18
#define LED_GREEN_PIN 19
#define LED_RED_PIN 23
#define LED_OUTPUT_MASK ((1ULL<<LED_BLUE_PIN) | (1ULL<<LED_GREEN_PIN) | (1ULL<<LED_RED_PIN))

// Function Prototypes
void gpio_initialize_output_led();
void initialize_gpio_w_isr(gpio_config_t *cfg);
void gpio_startup_routine(SemaphoreHandle_t *p_xDataReadySemaphore);

void vLEDBlueState();
void vLEDRedState();
void vLEDPurpleState();
void vLEDGreenState();