#ifndef DEFINITIONS_H___
#define DEFINITIONS_H___

//#define NO_GLOBAL_SERIAL                  // defined in compile flags
;                                          //
;                                          // #define DISABLE_SLEEP // for testing
;                                          //
#define SETTINGS_STR_LENGTH 32             // maximum length of settings strings
;                                          //
#define SLEEP_DELAY_DEFAULT 5 * 60 * 1000  // 5m
#define SLEEP_DELAY_MIN 1 * 60 * 1000      // 1m
#define SLEEP_COUNTDOWN_AFTER 30 * 1000    // 30s
#define SLEEP_COUNTDOWN_EVERY 2000         // 2s
;                                          //
#define MPU_RINGBUF_SIZE 16                // 128 ms smoothing @ 125 sps // TODO unused
#define STRAIN_RINGBUF_SIZE 512            // 80 sps @ 10 rpm = 480 samples/rev
#define POWER_RINGBUF_SIZE 2               // 3 sec smoothing @ 100 rpm
#define WIFISERIAL_RINGBUF_RX_SIZE 256     //
#define WIFISERIAL_RINGBUF_TX_SIZE 1024    // largest string to be printed should fit
;                                          //
#define HOSTNAME "ESPM"                    // default host name, used by wifiserial, ble and ota mdns
;                                          //
#define LED_PIN GPIO_NUM_22                // onboard LED pin
#define BATTERY_PIN GPIO_NUM_35            // pin for battery voltage measurement
;                                          //
#define MPU_SDA_PIN GPIO_NUM_23            //
#define MPU_SCL_PIN GPIO_NUM_33            //
#define MPU_WOM_INT_PIN GPIO_NUM_4         // rtc gpio for wake-on-motion interrupt
;                                          //
#define STRAIN_DOUT_PIN GPIO_NUM_5         //
#define STRAIN_SCK_PIN GPIO_NUM_13         //
;                                          //
#define HX711_SAMPLES 64                   // number of samples in moving average dataset, value must be 1, 2, 4, 8, 16, 32, 64 or 128.
#define HX711_IGN_HIGH_SAMPLE 1            // adds extra sample(s) to the dataset and ignores peak high/low sample, value must be 0 or 1.
#define HX711_IGN_LOW_SAMPLE 1             //
#define HX711_SCK_DELAY 1                  // microsecond delay after writing sck pin high or low. This delay could be required for faster mcu's.
;                                          //
#define MDM_HALL 0                         // use built-in hall sensor to detect crank revolutions
#define MDM_MPU 1                          // use MPU to detect crank revolutions
#define MOTION_DETECTION_METHOD MDM_HALL   // method of detecting crank revolutions
#define HALL_DEFAULT_THRESHOLD 10          // hall effect sensor default high threshold
#define HALL_DEFAULT_THRES_LOW 2           // hall effect sensor default low threshold
#define HALL_DEFAULT_OFFSET -100           // hall effect sensor default calibration offset
#define HALL_DEFAULT_SAMPLES 10            // # of samples for hall measurement averaging
#endif