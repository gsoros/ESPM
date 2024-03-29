#ifndef DEFINITIONS_H___
#define DEFINITIONS_H___

;                                           // #define NO_GLOBAL_SERIAL // defined in compile flags
;                                           //
;                                           // #define DISABLE_SLEEP // for testing
;                                           //
#define SETTINGS_STR_LENGTH 32              // maximum length of settings strings
#define BLE_CHAR_VALUE_MAXLENGTH 256        // maximum number of bytes written to ble characteristic values
;                                           //
;                                           // task frequencies in Hz
#define BOARD_TASK_FREQ 2.0f                //
#define WIFISERIAL_TASK_FREQ 10.0f          //
#define BLE_SERVER_TASK_FREQ 1.0f           //
#define BATTERY_TASK_FREQ 1.0f              //
#define MOTION_TASK_FREQ 125.0f             //
#define MPU_TEMP_TASK_FREQ 1.0f             //
#define STRAIN_TASK_FREQ 90.0f              //
#define POWER_TASK_FREQ 90.0f               //
#define OTA_TASK_FREQ 1.0f                  //
#define LED_TASK_FREQ 10.0f                 //
;                                           //
#define SLEEP_DELAY_DEFAULT 5 * 60 * 1000   // 5m
#define SLEEP_DELAY_MIN 1 * 60 * 1000       // 1m
#define SLEEP_COUNTDOWN_AFTER 30 * 1000     // 30s countdown on the serial console
#define SLEEP_COUNTDOWN_EVERY 2000          // 2s
;                                           //
#define MPU_RINGBUF_SIZE 16                 // 128 ms smoothing @ 125 sps // TODO unused
#define STRAIN_RINGBUF_SIZE 512             // 80 sps @ 10 rpm = 480 samples/rev
#define POWER_RINGBUF_SIZE 2                // 3 sec smoothing @ 100 rpm
#define WIFISERIAL_RINGBUF_RX_SIZE 256      //
#define WIFISERIAL_RINGBUF_TX_SIZE 1024     // largest string to be printed should fit
#define BATTERY_RINGBUF_SIZE 10             //
;                                           //
#define HOSTNAME "ESPM"                     // default host name, used by wifiserial, ble and ota mdns
;                                           //
#define LED_PIN GPIO_NUM_22                 // onboard LED pin
#define BATTERY_PIN GPIO_NUM_35             // pin for battery voltage measurement
;                                           //
#define MPU_SDA_PIN GPIO_NUM_23             //
#define MPU_SCL_PIN GPIO_NUM_33             //
#define MPU_WOM_INT_PIN GPIO_NUM_4          // rtc gpio for wake-on-motion interrupt
;                                           //
#define STRAIN_DOUT_PIN GPIO_NUM_5          //
#define STRAIN_SCK_PIN GPIO_NUM_13          //
;                                           //
#define TEMPERATURE_PIN GPIO_NUM_32         // onewire ds18b20 parasitic
;                                           //
#define HX711_SAMPLES 64                    // number of samples in moving average dataset, value must be 1, 2, 4, 8, 16, 32, 64 or 128.
#define HX711_IGN_HIGH_SAMPLE 1             // adds extra sample(s) to the dataset and ignores peak high/low sample, value must be 0 or 1.
#define HX711_IGN_LOW_SAMPLE 1              //
#define HX711_SCK_DELAY 1                   // microsecond delay after writing sck pin high or low. This delay could be required for faster MCUs.
;                                           //
#define MDM_HALL 0                          // use built-in hall sensor to detect crank revolutions
#define MDM_MPU 1                           // use MPU to detect crank revolutions
#define MDM_STRAIN 2                        // use strain gauge to detect crank revolutions
#define MDM_MAX 3                           // marks the high limit
#define MOTION_DETECTION_METHOD MDM_STRAIN  // method of detecting crank revolutions
#define MDM_STRAIN_DEFAULT_THRESHOLD 10     // strain motion detection default high threshold
#define MDM_STRAIN_DEFAULT_THRES_LOW 2      // strain motion detection default low threshold
;                                           //
#define HALL_DEFAULT_THRESHOLD 10           // hall effect sensor default high threshold
#define HALL_DEFAULT_THRES_LOW 2            // hall effect sensor default low threshold
#define HALL_DEFAULT_OFFSET -100            // hall effect sensor default calibration offset
#define HALL_DEFAULT_SAMPLES 10             // # of samples for hall measurement averaging
;                                           //
#define NTM_KEEP 0                          // include negative torque readings in the power calculation
#define NTM_ZERO 1                          // convert negative torque readings to zero
#define NTM_DISCARD 2                       // discard negative torque readings
#define NTM_ABS 3                           // use the absolute value of negative torque readings
#define NTM_MAX 4                           // marks the high limit
#define NEGATIVE_TORQUE_METHOD NTM_ZERO     // method of dealing with negative torque readings
;                                           //
#define CRANK_EVENT_MIN_MS 400              // 400 ms = 150 RPM
;                                           //
#define POWER_ZERO_DELAY_MS 3000            // push zero power values after the last crank event (3s = 20RPM)
;                                           //
#define AUTO_TARE 1                         // enable auto tare by default
#define AUTO_TARE_DELAY_MS 3000             // auto tare after 3 secs of last crank event
#define AUTO_TARE_RANGE_G 1000              // buffer values must within this range (1 kg) for auto tare
;                                           //
#define WM_OFF 0                            // weight scale measurement characteristic updates disabled
#define WM_ON 1                             // enabled
#define WM_WHEN_NO_CRANK 2                  // enabled while there are no crank events
#define WM_MAX 3                            // marks the high limit
#define WM_CHAR_MODE WM_WHEN_NO_CRANK       //
;                                           //

#include "atoll_ble_constants.h"

#endif