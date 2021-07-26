#ifndef DEFINITIONS_H___
#define DEFINITIONS_H___

//#define NO_GLOBAL_SERIAL               // defined in compile flags
;                                        //
;                                        // #define DISABLE_SLEEP // for testing
;                                        //
#define MPU_RINGBUF_SIZE 16              // 128 ms smoothing @ 125 sps
#define STRAIN_RINGBUF_SIZE 512          // 80 sps @ 10 rpm = 480 samples/rev
#define POWER_RINGBUF_SIZE 2             // 3 sec smoothing @ 100 rpm
#define WIFISERIAL_RINGBUF_RX_SIZE 256   //
#define WIFISERIAL_RINGBUF_TX_SIZE 1024  // largest string to be printed should fit
;                                        //
#define HOSTNAME "ESPM"                  // default host name, used by wifiserial, ble and ota mdns
;                                        //
#define LED_PIN GPIO_NUM_22              // onboard LED pin
#define BATTERY_PIN GPIO_NUM_35          // pin for battery voltage measurement
;                                        //
#define MPU_SDA_PIN GPIO_NUM_23          //
#define MPU_SCL_PIN GPIO_NUM_33          //
#define MPU_WOM_INT_PIN GPIO_NUM_4       // rtc gpio for wake-on-motion interrupt
;                                        //
#define STRAIN_DOUT_PIN GPIO_NUM_5       //
#define STRAIN_SCK_PIN GPIO_NUM_13       //
;                                        //
#define HX711_SAMPLES 64                 // number of samples in moving average dataset, value must be 1, 2, 4, 8, 16, 32, 64 or 128.
#define HX711_IGN_HIGH_SAMPLE 1          // adds extra sample(s) to the dataset and ignores peak high/low sample, value must be 0 or 1.
#define HX711_IGN_LOW_SAMPLE 1           //
#define HX711_SCK_DELAY 1                // microsecond delay after writing sck pin high or low. This delay could be required for faster mcu's.

#define BOOTMODE_INVALID -1
#define BOOTMODE_LIVE 1
#define BOOTMODE_LIVE_S "live"
#define BOOTMODE_OTA 2
#define BOOTMODE_OTA_S "ota"
#define BOOTMODE_DEBUG 3
#define BOOTMODE_DEBUG_S "debug"

#endif