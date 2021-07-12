#ifndef DEFINITIONS_H___
#define DEFINITIONS_H___

//#define NO_GLOBAL_SERIAL // defined in compile flags

#define MPU_RINGBUF_SIZE 16
#define POWER_RINGBUF_SIZE 96
#define WIFISERIAL_RINGBUF_RX_SIZE 256
#define WIFISERIAL_RINGBUF_TX_SIZE 1024  // largest string to be printed should fit

#define HOSTNAME "ESPM"  // default host name

#define LED_PIN GPIO_NUM_22      // onboard LED pin
#define BATTERY_PIN GPIO_NUM_35  // pin for battery voltage measurement

#define MPU_SDA_PIN GPIO_NUM_23
#define MPU_SCL_PIN GPIO_NUM_33
#define MPU_WOM_INT_PIN GPIO_NUM_4  // rtc gpio for wake-on-motion interrupt

#define STRAIN_DOUT_PIN GPIO_NUM_5
#define STRAIN_SCK_PIN GPIO_NUM_13

#define HX711_SAMPLES 16         // number of samples in moving average dataset, value must be 1, 2, 4, 8, 16, 32, 64 or 128.
#define HX711_IGN_HIGH_SAMPLE 1  // adds extra sample(s) to the dataset and ignore peak high/low sample, value must be 0 or 1.
#define HX711_IGN_LOW_SAMPLE 1   //
#define HX711_SCK_DELAY 1        // microsecond delay after writing sck pin high or low. This delay could be required for faster mcu's.

#endif