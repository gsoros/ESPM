#ifndef DEFINITIONS_H___
#define DEFINITIONS_H___

//#define NO_GLOBAL_SERIAL

#define MPU_RINGBUF_SIZE 16
#define POWER_RINGBUF_SIZE 96
#define WIFISERIAL_RINGBUF_RX_SIZE 256
#define WIFISERIAL_RINGBUF_TX_SIZE 1024

#define HOSTNAME "ESPM"

//#define LED_PIN 22
#define BATTERY_PIN 35  // pin for battery voltage measurement

#define MPU_SDA_PIN GPIO_NUM_23
#define MPU_SCL_PIN GPIO_NUM_19
#define MPU_WOM_INT_PIN GPIO_NUM_4  // interrupt pin for wake-on-motion

#define STRAIN_DOUT_PIN GPIO_NUM_5
#define STRAIN_SCK_PIN GPIO_NUM_13

#endif