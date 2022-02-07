#include <stdio.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/projdefs.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"

#include "lsm6dsr.h"



spi_device_handle_t* lsm6dsr_initialize_spi(spi_bus_config_t *buscfg, spi_device_interface_config_t *devcfg)
{
  spi_device_handle_t *p_lsm6dsr_spi_handle;
  p_lsm6dsr_spi_handle = (spi_device_handle_t*)malloc(sizeof(spi_device_handle_t));
  ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, buscfg, SPI_DMA_CH_AUTO));
  ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, devcfg, p_lsm6dsr_spi_handle));
  return p_lsm6dsr_spi_handle;
};

void lsm6dsr_write_register(spi_device_handle_t *spi, char address, char value)
{
  // Send SPI Transaction 
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  char sendbuf[2] = {address, value};
  char recvbuf[2];
  t.length = 16;
  t.tx_buffer = sendbuf;
  t.rx_buffer = recvbuf;
  ESP_ERROR_CHECK(spi_device_polling_transmit(*spi, &t));
};

char lsm6dsr_read_register(spi_device_handle_t *spi, char address)
{
  // Send SPI Transaction 
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  char sendbuf[2] = {(address | 0x80), 0x00};
  char recvbuf[2];
  t.length = 16;
  t.tx_buffer = sendbuf;
  t.rx_buffer = recvbuf;
  ESP_ERROR_CHECK(spi_device_polling_transmit(*spi, &t));
  return recvbuf[1];
};

void lsm6dsr_read_single_xl_measurement(spi_device_handle_t *spi, char *recvbuf)
{
  // Prep SPI Transaction
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  char sendbuf[7];
  sendbuf[0] = (OUTX_L_A | 0x80);
  t.length = (7*8);
  t.tx_buffer = sendbuf;
  t.rx_buffer = recvbuf;
  ESP_ERROR_CHECK(spi_device_polling_transmit(*spi, &t));
}

void lsm6dsr_parse_read_data_buffer(char *p_read_data_buffer, int16_t *p_acc_xyz_data_buffer)
{
  // Extract Data
  uint16_t acc_xyz_data[3];
  acc_xyz_data[0] = ((*(p_read_data_buffer+2)) << 8 | (*(p_read_data_buffer+1)));
  acc_xyz_data[1] = ((*(p_read_data_buffer+4)) << 8 | (*(p_read_data_buffer+3)));
  acc_xyz_data[2] = ((*(p_read_data_buffer+6)) << 8 | (*(p_read_data_buffer+5)));
  int16_t acc_xyz_data_signed[3];

  for (int i=0; i < 3; i++)
  {
    if (acc_xyz_data[i] > 32767)
    {
      acc_xyz_data[i] = ~acc_xyz_data[i];
      acc_xyz_data[i]++;
      acc_xyz_data_signed[i] =  -((int16_t)acc_xyz_data[i]);
    }
    else
      acc_xyz_data_signed[i] = (int16_t) acc_xyz_data[i];
    *(p_acc_xyz_data_buffer+i) = acc_xyz_data_signed[i];
  }
}

bool lsm6dsr_check_data_ready(spi_device_handle_t *spi)
{
  // Send SPI Transaction 
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  char sendbuf[2] = {(STATUS_REG | 0x80), 0x00};
  char recvbuf[2];
  t.length = 16;
  t.tx_buffer = sendbuf;
  t.rx_buffer = recvbuf;
  ESP_ERROR_CHECK(spi_device_transmit(*spi, &t));
  recvbuf[1] &= 0x01;
  bool data_ready = recvbuf[1];
  return data_ready;
}

void lsm6dsr_batch_read_fifo(spi_device_handle_t *spi, int watermark_threshold, char *p_fifo_data_buffer)
{
  // Send SPI Transaction 
  spi_transaction_t t;
  memset(&t, 0, sizeof(t)); 
  char sendbuf[(7*watermark_threshold)+1];
  sendbuf[0] = (FIFO_DATA_OUT_TAG | 0x80);

  // char *recvbuf = (char*)malloc( (sizeof(char)*7*watermark_threshold) + sizeof(char));
  t.length = (7*8*watermark_threshold)+8;
  t.tx_buffer = sendbuf;
  t.rx_buffer = p_fifo_data_buffer;
  ESP_ERROR_CHECK(spi_device_polling_transmit(*spi, &t));
};

char lsm6dsr_write_read_register(spi_device_handle_t *spi, char address, char value)
{
  // Write SPI Transaction 
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  char sendbuf[2] = {address, value};
  char recvbuf[2];
  t.length = 16;
  t.tx_buffer = sendbuf;
  t.rx_buffer = recvbuf;
  ESP_ERROR_CHECK(spi_device_polling_transmit(*spi, &t));

  // Read SPI Transaction
  sendbuf[0] = (address | 0x80);
  ESP_ERROR_CHECK(spi_device_polling_transmit(*spi, &t));
  return recvbuf[1];
}

void lsm6dsr_initialize_accelerometer(spi_device_handle_t *spi, lsm6dsr_accelerometer_config_t *cfg)
{
  // Generate register command from configuration structure
  static const char *TAG = "LSM6DSR Accelerometer Initialization";
  // Prep CTRL1_XL Register Command
  char CTRL1_XL_command = 0x00;

  // Set bits |7|6|5|4|
  char ODR_XL_bits = (cfg->rate) << 4;
  // Set bits |3|2|
  char FSN_XL_bits = (cfg->resolution) << 2;

  // Set CTRL1_XL register command
  CTRL1_XL_command = ODR_XL_bits | FSN_XL_bits;

  // Send SPI Transaction 
  char register_value = lsm6dsr_write_read_register(spi, CTRL1_XL, CTRL1_XL_command);
  ESP_LOGI(TAG, "CTRL1_XL set to: %x", register_value);
};

void lsm6dsr_initialize_gyroscope(spi_device_handle_t *spi, lsm6dsr_gyroscope_config_t *cfg)
{
  static const char *TAG = "LSM6DSR Gyroscope Initialization";

  // Prep CTRL2_G Register Command
  char CTRL2_G_command = (0x00 | (cfg->resolution)) | ((cfg->rate) << 4);
  char register_value = lsm6dsr_write_read_register(spi, CTRL2_G, CTRL2_G_command);
  ESP_LOGI(TAG, "CTRL2_G set to: %x", register_value);
}

void lsm6dsr_initialize_activity_inactivity_interrupt(spi_device_handle_t *spi, lsm6dsr_activity_inactivity_config_t *cfg)
{
  static const char *TAG = "LSM6DSR Activity/Inactivity Initialization";
  // Prep WAKE_UP_DUR Register Command
  char WAKE_UP_DUR_command = 0x00;

  // Set bits |6|5|
  char WAKE_DUR_bits = (cfg->activity_time) << 5;
  // Set bit |4|
  char WAKE_THS_W_bits = (cfg->activity_inactivity_thresh_LSB_size) << 4;
  // Set bits |3|2|1|0|
  char SLEEP_DUR_bits  = (cfg->inactivity_time_ODRs);

  // Set WAKE_UP_DUR register command
  WAKE_UP_DUR_command = WAKE_DUR_bits | WAKE_THS_W_bits | SLEEP_DUR_bits;
  char register_value = lsm6dsr_write_read_register(spi, WAKE_UP_DUR, WAKE_UP_DUR_command);
  ESP_LOGI(TAG, "WAKE_UP_DUR set to: %x", register_value);

  // Prep WAKE_UP_THS Register (Wake Up Thresholds)
  char WAKE_UP_THS_command = 0x00;

  // Set bits 4|3|2|1|0|
  char WK_THS_bits = (cfg->activity_inactivity_thresh_LSBs);

  // Set WAKE_UP_THS register command
  WAKE_UP_THS_command |= WK_THS_bits;
  register_value = lsm6dsr_write_read_register(spi, WAKE_UP_THS, WAKE_UP_THS_command);
  ESP_LOGI(TAG, "WAKE_UP_THS set to: %x", register_value);

  // Set TAP_CFG0 register
  char TAP_CFG0_command = 0x00;
  register_value = lsm6dsr_write_read_register(spi, TAP_CFG0, TAP_CFG0_command);
  ESP_LOGI(TAG, "TAP_CFG0 set to: %x", register_value);

  // Set TAP_CFG2 register
  char TAP_CFG2_command = 0xC0;
  register_value = lsm6dsr_write_read_register(spi, TAP_CFG2, TAP_CFG2_command);
  ESP_LOGI(TAG, "TAP_CFG2 set to: %x", register_value);

  // Set MD1_CFG register
  char MD1_CFG_command = 0x80;
  register_value = lsm6dsr_write_read_register(spi, MD1_CFG, MD1_CFG_command);
  ESP_LOGI(TAG, "MD1_CFG set to: %x", register_value);

  // Calculate Inactivity Time and Activity/Inactivity Threshold
  register_value = lsm6dsr_read_register(spi, CTRL1_XL);
  char resolution_bits = (register_value & 0x0A) >> 2;
  double resolution_value;
  switch(resolution_bits)
  {
    case 0:
      resolution_value = 2;
      break;
    case 1:
      resolution_value = 16;
      break;
    case 2:
      resolution_value = 4;
      break;
    case 3:
      resolution_value = 8;
      break;
    default:
      resolution_value = 0;
      break;
  }
  double data_rate = pow( 2, (((register_value & 0xF0) >> 4)-1)) * 13;
  double sleep_duration = (((double)SLEEP_DUR_bits * 512) / data_rate);
  ESP_LOGI(TAG, "Inactivity Timer set to: %.2f", sleep_duration);

  double activity_inactivity_threshold = ((double)WK_THS_bits * resolution_value) / pow(2, (6+(2*(WAKE_THS_W_bits >> 4))));
  ESP_LOGI(TAG, "Differential Threshold set to: %.2fg", activity_inactivity_threshold);

}

void lsm6dsr_initialize_FIFO(spi_device_handle_t *spi, lsm6dsr_fifo_config_t *cfg)
{
  // Set logging tag 
  static const char *TAG = "LSM6DSR FIFO Initialization";

  // FIFO_CTRL1 Register Command
  char FIFO_CTRL1_command = 0x00 | (cfg->watermark_byte_threshold);
  char register_value1 = lsm6dsr_write_read_register(spi, FIFO_CTRL1, FIFO_CTRL1_command);
  char FIFO_CTRL2_command = 0x00 | ((cfg->watermark_byte_threshold) >> 8);
  char register_value2 = lsm6dsr_write_read_register(spi, FIFO_CTRL2, FIFO_CTRL2_command);
  int watermark_value = (register_value2 << 8) | register_value1;
  ESP_LOGI(TAG, "FIFO Watermark Threshold set to: %d", watermark_value);

  // Set Watermark Interrupt
  char register_value;
  switch((cfg->watermark_interrupt_setting))
  {
    case WATERMARK_INT1_ON:
    {
      char INT1_CTRL_current_setting = lsm6dsr_read_register(spi, INT1_CTRL);
      char INT1_CTRL_command = ((0x00 | INT1_CTRL_current_setting) | 0x08);
      register_value = lsm6dsr_write_read_register(spi, INT1_CTRL, INT1_CTRL_command);
      ESP_LOGI(TAG, "INT1_CTRL set to: %x", register_value);
      break;
    }
    case WATERMARK_INT2_ON:
    {
      char INT2_CTRL_current_setting = lsm6dsr_read_register(spi, INT2_CTRL);
      char INT2_CTRL_command = ((0x00 | INT2_CTRL_current_setting) | 0x08);
      register_value = lsm6dsr_write_read_register(spi, INT2_CTRL, INT2_CTRL_command);
      ESP_LOGI(TAG, "INT2_CTRL set to: %x", register_value);
      break;
    }
    default:
      break;
  }

  // Set FIFO Write speed datarates
  // Read current ODR's from CTRL1_XL and CTRL2_GY
  char ODR_XL_current = lsm6dsr_read_register(spi, CTRL1_XL);
  ODR_XL_current = (0xF0 & ODR_XL_current) >> 4;
  char ODR_G_current = lsm6dsr_read_register(spi, CTRL2_G);
  ODR_G_current = (0xF0 & ODR_G_current);

  // Prep and send command for FIFO_CTRL3 register (Set FIFO ODRs)
  char FIFO_CTRL3_command = (0x00 | ODR_XL_current) | ODR_G_current;
  register_value = lsm6dsr_write_read_register(spi, FIFO_CTRL3, FIFO_CTRL3_command);
  ESP_LOGI(TAG, "FIFO_CTRL3 set to: %x", register_value);

  // Place FIFO in selected mode 
  char FIFO_CTRL4_command = ((lsm6dsr_read_register(spi, FIFO_CTRL4) & 0xF8) | (cfg->fifo_mode));
  register_value = lsm6dsr_write_read_register(spi, FIFO_CTRL4, FIFO_CTRL4_command);
  ESP_LOGI(TAG, "FIFO_CTRL4 set to: %x", register_value);
}

void lsm6dsr_FIFO_start(spi_device_handle_t *spi)
{
  lsm6dsr_write_register(spi, FIFO_CTRL4, 0x05);
}

void lsm6dsr_FIFO_stop(spi_device_handle_t *spi)
{
  lsm6dsr_write_register(spi, FIFO_CTRL4, 0x00); 
}

// Create lsm6dsr state change methods
void lsm6dsr_enter_sleep_active_state(spi_device_handle_t *spi)
{

  // Turn off all interrupts
  lsm6dsr_write_register(spi, INT1_CTRL, 0x00);

  // Turn off XL
  lsm6dsr_write_register(spi, CTRL1_XL, 0x08);

  // Turn off Gyroscope
  lsm6dsr_write_register(spi, CTRL2_G, 0x04);

  // Set to low power
  lsm6dsr_write_register(spi, CTRL6_C, 0x10);

  // Set Data Ready Interrupt
  lsm6dsr_write_register(spi, INT1_CTRL, 0x01);

  // Set ODR to 52Hz
  lsm6dsr_write_register(spi, CTRL1_XL, 0x38);

}


/********************************************************
************ Startup Routine Code ***********************
********************************************************/

// Set define constants
#define PIN_MISO       12
#define PIN_MOSI       13
#define PIN_CLK        14
#define PIN_CS         27
#define WATERMARK_THRESHOLD 300

spi_device_handle_t* lsm6dsr_startup_routine()
{
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
    .clock_speed_hz   = 1E5,
    .duty_cycle_pos   = 128,
    .mode             = 3,
    .spics_io_num     = PIN_CS,
    .cs_ena_posttrans = 3,
    .queue_size       = 3
  };
  spi_device_handle_t *p_lsm6dsr_spi_handle = lsm6dsr_initialize_spi(&buscfg, &devcfg);

  // Initialize Accelerometer
  lsm6dsr_accelerometer_config_t acc_config = {
    .rate       = ODR_3330HZ,
    .resolution = PLUS_MINUS_4G, 
  };
  lsm6dsr_initialize_accelerometer(p_lsm6dsr_spi_handle, &acc_config);

  // Initialize Gyroscope
  lsm6dsr_gyroscope_config_t gyro_config = {
    .rate = ODR_3330HZ,
    .resolution = PLUS_MINUS_500DPS,
  };
  lsm6dsr_initialize_gyroscope(p_lsm6dsr_spi_handle, &gyro_config);

  // // Initialize Activity/Inactivity Interrupt
  // lsm6dsr_activity_inactivity_config_t activity_config = {
  //   .activity_time = ACTIVITY_TIME_320ms,
  //   .activity_inactivity_thresh_LSB_size = LSB_EQUALS_RESOLUTION_OVER_256,
  //   .activity_inactivity_thresh_LSBs = 5,
  //   .inactivity_time_ODRs = INACTIVITY_TIME_7680_ODRs,
  // };
  // lsm6dsr_initialize_activity_inactivity_interrupt(p_lsm6dsr_spi_handle, &activity_config);

  // Initialize FIFO
  lsm6dsr_fifo_config_t fifo_config = {
    .watermark_interrupt_setting = WATERMARK_INT2_ON,
    .watermark_byte_threshold = WATERMARK_THRESHOLD,
    .fifo_mode = BYPASS_MODE, 
  };
  lsm6dsr_initialize_FIFO(p_lsm6dsr_spi_handle, &fifo_config);

  return p_lsm6dsr_spi_handle;
}










