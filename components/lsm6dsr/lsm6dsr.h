#pragma once
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

// Set LSM6DSR Register Constants
#define FUNC_CFG_ACCESS                 (0x01)
#define PIN_CTRL                        (0x02)
#define S4S_TPH_L                       (0x04)
#define S4S_TPH_H                       (0x05)
#define S4S_RR                          (0x06)
#define FIFO_CTRL1                      (0x07)
#define FIFO_CTRL2                      (0x08)
#define FIFO_CTRL3                      (0x09)
#define FIFO_CTRL4                      (0x0A)
#define COUNTER_BDR_REG1                (0x0B)
#define COUNTER_BDR_REG2                (0x0C)
#define INT1_CTRL                       (0x0D)
#define INT2_CTRL                       (0x0E)
#define WHO_AM_I                        (0x0F)
#define CTRL1_XL                        (0x10)
#define CTRL2_G                         (0x11)
#define CTRL3_C                         (0x12)
#define CTRL4_C                         (0x13)
#define CTRL5_C                         (0x14)
#define CTRL6_C                         (0x15)
#define CTRL7_G                         (0x16)
#define CTRL8_XL                        (0x17)
#define CTRL9_XL                        (0x18)
#define CTRL10_C                        (0x19)
#define ALL_INT_SRC                     (0x1A)
#define WAKE_UP_SRC                     (0x1B)
#define TAP_SRC                         (0x1C)
#define D6D_SRC                         (0x1D)
#define STATUS_REG                      (0x1E)
#define OUT_TEMP_L                      (0x20)
#define OUT_TEMP_H                      (0x21)
#define OUTX_L_G                        (0x22)
#define OUTX_H_G                        (0x23)
#define OUTY_L_G                        (0x24)
#define OUTY_H_G                        (0x25)
#define OUTZ_L_G                        (0x26)
#define OUTZ_H_G                        (0x27)
#define OUTX_L_A                        (0x28)
#define OUTX_H_A                        (0x29)
#define OUTY_L_A                        (0x2A)
#define OUTY_H_A                        (0x2B)
#define OUTZ_L_A                        (0x2C)
#define OUTZ_H_A                        (0x2D)
#define EMB_FUNC_STATUS_MAINPAGE        (0x35)
#define FSM_STATUS_A_MAINPAGE           (0x36)
#define FSM_STATUS_B_MAINPAGE           (0x37)
#define STATUS_MASTER_MAINPAGE          (0x39)
#define FIFO_STATUS1                    (0x3A)
#define FIFO_STATUS2                    (0x3B)
#define TIMESTAMP0                      (0x40)
#define TIMESTAMP1                      (0x41)
#define TIMESTAMP2                      (0x42)
#define TIMESTAMP3                      (0x43)
#define TAP_CFG0                        (0x56)
#define TAP_CFG1                        (0x57)
#define TAP_CFG2                        (0x58)
#define TAP_THS_6D                      (0x59)
#define INT_DUR2                        (0x5A)
#define WAKE_UP_THS                     (0x5B)
#define WAKE_UP_DUR                     (0x5C)
#define FREE_FALL                       (0x5D)
#define MD1_CFG                         (0x5E)
#define MD2_CFG                         (0x5F)
#define S4S_ST_CMD_CODE                 (0x60)
#define S4S_DT_REG                      (0x61)
#define I3C_BUS_AVB                     (0x62)
#define INTERNAL_FREQ_FINE              (0x63)
#define INT_OIS                         (0x6F)
#define CTRL1_OIS                       (0x70)
#define CTRL2_OIS                       (0x71)
#define CTRL3_OIS                       (0x72)
#define X_OFS_USR                       (0x73)
#define Y_OFS_USR                       (0x74)
#define Z_OFS_USR                       (0x75)
#define FIFO_DATA_OUT_TAG               (0x78)
#define FIFO_DATA_OUT_X_L               (0x79)
#define FIFO_DATA_OUT_X_H               (0x7A)
#define FIFO_DATA_OUT_Y_L               (0x7B)
#define FIFO_DATA_OUT_Y_H               (0x7C)
#define FIFO_DATA_OUT_Z_L               (0x7D)
#define FIFO_DATA_OUT_Z_H               (0x7E)


// Data Structures
/**
 * @brief LSM6DSR Data Rate Options Enum
*/
typedef enum {
  POWER_DOWN = 0,
  ODR_12HZ   = 1,
  ODR_26HZ   = 2,
  ODR_52HZ   = 3,
  ODR_104HZ  = 4,
  ODR_208HZ  = 5,
  ODR_416HZ  = 6,
  ODR_833HZ  = 7,
  ODR_1660HZ = 8,
  ODR_3330HZ = 9,
  ODR_6660HZ = 10,
} lsm6dsr_data_rate_enum_t;

/**
 * @brief LSM6DSR Resolution Options Enum
*/
typedef enum {
  PLUS_MINUS_2G  = 0,
  PLUS_MINUS_16G = 1,
  PLUS_MINUS_4G  = 2,
  PLUS_MINUS_8G  = 3,
} lsm6dsr_acc_resolution_enum_t; 

/**
 * @brief LSM6DSR CTRL1_XL Configuration Structure
*/
typedef struct lsm6dsr_accelerometer_config_t {
  lsm6dsr_data_rate_enum_t rate;
  lsm6dsr_acc_resolution_enum_t resolution;
}lsm6dsr_accelerometer_config_t;

typedef enum {
  PLUS_MINUS_4000DPS = 1,
  PLUS_MINUS_125DPS  = 2, 
  PLUS_MINUS_250DPS  = 0,
  PLUS_MINUS_500DPS  = 4,
  PLUS_MINUS_1000DPS = 8,
  PLUS_MINUS_2000DPS = 12,
} lsm6dsr_gyro_resolution_enum_t;

typedef struct lsm6dsr_gyroscope_config_t {
  lsm6dsr_data_rate_enum_t rate;
  lsm6dsr_gyro_resolution_enum_t resolution;
} lsm6dsr_gyroscope_config_t;
/**
 * @brief LSM6DSR Waketime Options Enum
*/
typedef enum {
  ACTIVITY_TIME_80ms  = 0,
  ACTIVITY_TIME_160ms = 1,
  ACTIVITY_TIME_240ms = 2,
  ACTIVITY_TIME_320ms = 3,
} lsm6dsr_activity_time_enum_t;

/**
 * @brief LSM6DSR Wake Slope Threshold Options
*/
typedef enum {
  LSB_EQUALS_RESOLUTION_OVER_64  = 0,
  LSB_EQUALS_RESOLUTION_OVER_256 = 1,
} lsm6dsr_activity_inactivity_thresh_LSB_size_enum_t;

typedef enum {
  INACTIVITY_TIME_16_ODRs   = 0,
  INACTIVITY_TIME_512_ODRs  = 1,
  INACTIVITY_TIME_1024_ODRs = 2,
  INACTIVITY_TIME_1536_ODRs = 3,
  INACTIVITY_TIME_2048_ODRs = 4,
  INACTIVITY_TIME_2560_ODRs = 5,
  INACTIVITY_TIME_3072_ODRs = 6,
  INACTIVITY_TIME_3584_ODRs = 7,
  INACTIVITY_TIME_4096_ODRs = 8,
  INACTIVITY_TIME_4608_ODRs = 9,
  INACTIVITY_TIME_5120_ODRs = 10,
  INACTIVITY_TIME_5632_ODRs = 11,
  INACTIVITY_TIME_6144_ODRs = 12,
  INACTIVITY_TIME_6656_ODRs = 13,
  INACTIVITY_TIME_7168_ODRs = 14,
  INACTIVITY_TIME_7680_ODRs = 15,
} lsm6dsr_inactivity_time_enum_t;
/**
 * @brief LSM6DSR Activity/Inactivity Congifuration Structure
 * Note: 
*/
typedef struct lsm6dsr_activity_inactivity_config_t {
  lsm6dsr_activity_time_enum_t activity_time;
  lsm6dsr_activity_inactivity_thresh_LSB_size_enum_t activity_inactivity_thresh_LSB_size;
  char activity_inactivity_thresh_LSBs;
  lsm6dsr_inactivity_time_enum_t inactivity_time_ODRs;

} lsm6dsr_activity_inactivity_config_t;

typedef enum {
  WATERMARK_INT_OFF = 0,
  WATERMARK_INT1_ON = 1,
  WATERMARK_INT2_ON = 2,
} lsm6dsr_watermark_interrupt_enum_t;

typedef enum {
  BYPASS_MODE               = 0,
  FIFO_MODE                 = 1,
  CONTINUOUS_TO_FIFO_MODE   = 3,
  BYPASS_TO_CONTINUOUS_MODE = 4,
  CONTINUOUS_MODE           = 6,
  BYPASS_TO_FIFO_MODE       = 7,
} lsm6dsr_fifo_mode_t;

// typedef enum {
//   FORCED_UNCOPTR_OFF  = 0x00;
//   EVERY_8_BATCHES  = 0x02;
//   EVERY_16_BATCHES = 0x04;
//   EVERY_32_BATCHES = 0x06;
// } lsm6dsr_uncoptr_rate_t

// typedef enum {
//   TRUE = 0x10;
//   FALSE = 0x00;
// } batch_odr_change_virtual_sensor_into_fifo_t

// typedef enum {
//   TRUE = 0x40;
//   FALSE = 0x00;
// } fifo_compression_t

// typedef enum {
//   TRUE = 0x80;
//   FALSE = 0x00;
// } stop_writing_on_watermark_t

// typedef enum {
//   BDR_XL_OFF = 0x00;
//   BDR_XL_1HZ = 0x0B;
//   BDR_XL_12HZ = 0x01;
//   BDR_XL_26HZ = 0x02;
//   BDR_XL_52HZ = 0x03;
//   BDR_XL_104HZ = 0x04;
//   BDR_XL_208HZ = 0x05;
//   BDR_XL_417HZ = 0x06;
//   BDR_XL_833HZ = 0x07;
//   BDR_XL_1667HZ = 0x08;
//   BDR_XL_3333HZ = 0x09;
//   BDR_XL_6667HZ = 0x0A;
// } fifo_batch_data_rate_xl_t

// typedef enum {
//   BDR_GYR_OFF = 0x00;
//   BDR_GYR_6HZ = 0xB0;
//   BDR_GYR_12HZ = 0x10;
//   BDR_GYR_26HZ = 0x20;
//   BDR_GYR_52HZ = 0x30;
//   BDR_GYR_104HZ = 0x40;
//   BDR_GYR_208HZ = 0x50;
//   BDR_GYR_417HZ = 0x60;
//   BDR_GYR_833HZ = 0x70;
//   BDR_GYR_1667HZ = 0x80;
//   BDR_GYR_3333HZ = 0x90;
//   BDR_GYR_6667HZ = 0xA0;
// } fifo_batch_data_rate_gyr_t


// typedef enum {
//    TEMPERATURE_BATCHING_OFF = 0x00;
//    TEMPERATURE_BATCHRATE_1HZ = 0x10;
//    TEMPERATURE_BATCHRATE_12HZ = 0x20;
//    TEMPERATURE_BATCHRATE_52HZ = 0x30;
// } lsm6dsr_temperature_batching_rate_t

// typedef enum {
//   TIMESTAMP_BATCHING_OFF = 0x00;
//   BATCHING_DECIMATION_1 = 0x40;
//   BATCHING_DECIMATION_8 = 0x80;
//   BATCHING_DECIMATION_32 = 0xC0;
// } lsm6dsr_timestamp_batching_decimation_t

typedef struct lsm6dsr_fifo_config_t {
  lsm6dsr_watermark_interrupt_enum_t watermark_interrupt_setting;
  short watermark_byte_threshold; // integer from 0 to 511
  lsm6dsr_fifo_mode_t fifo_mode;
  // lsm6dsr_uncoptr_rate_t forced_uncompressed_writing_rate;
  // batch_odr_change_virtual_sensor_into_fifo_t batch_odr_change_virtual_sensor_into_fifo;
  // fifo_compression_t fifo_compression;
  // stop_writing_on_watermark_t stop_writing_on_watermark;
  // fifo_batch_data_rate_xl_t fifo_batch_data_rate_xl; // write frequency of accelerometer to FIFO
  // fifo_batch_data_rate_gyr_t fifo_batch_data_rate_gyr; // write frequency of gyroschope to FIFO
  // lsm6dsr_temperature_batching_rate_t lsm6dsr_temperature_batching_rate;
  // lsm6dsr_timestamp_batching_decimation_t lsm6dsr_timestamp_batching_decimation;
  // short fifo_bdr_threshold; // 0 to 512
} lsm6dsr_fifo_config_t;

// Function Prototypes
spi_device_handle_t* lsm6dsr_startup_routine();

spi_device_handle_t* lsm6dsr_initialize_spi(spi_bus_config_t *buscfg, spi_device_interface_config_t *devcfg);
void lsm6dsr_initialize_accelerometer(spi_device_handle_t *spi, lsm6dsr_accelerometer_config_t *cfg);
void lsm6dsr_initialize_gyroscope(spi_device_handle_t *spi, lsm6dsr_gyroscope_config_t *cfg);
void lsm6dsr_initialize_activity_inactivity_interrupt(spi_device_handle_t *spi, lsm6dsr_activity_inactivity_config_t *cfg);
void lsm6dsr_enter_sleep_active_state(spi_device_handle_t *spi);
void lsm6dsr_read_single_xl_measurement(spi_device_handle_t *spi, char *recvbuf);
void lsm6dsr_parse_read_data_buffer(char *p_read_data_buffer, int16_t *p_acc_xyz_data_buffer);
bool lsm6dsr_check_data_ready(spi_device_handle_t *spi);
void lsm6dsr_FIFO_start(spi_device_handle_t *spi);
void lsm6dsr_FIFO_stop(spi_device_handle_t *spi);
void lsm6dsr_batch_read_fifo(spi_device_handle_t *spi, int watermark_threshold, char *p_fifo_data_buffer);
void lsm6dsr_initialize_FIFO(spi_device_handle_t *spi, lsm6dsr_fifo_config_t *cfg);

void lsm6dsr_write_register(spi_device_handle_t *spi, char address, char value);
char lsm6dsr_read_register(spi_device_handle_t *spi, char address);
char lsm6dsr_write_read_register(spi_device_handle_t *spi, char address, char value);




