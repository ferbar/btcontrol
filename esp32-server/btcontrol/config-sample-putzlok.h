//#define lok_name "esp32-playmobil"
//#define lok_name "esp32-playmobil-wemos"
#define lok_name "esp32-putzlok"
#include <config-wifi.h>

// enable update OTA
#define OTA_UPDATE

// bridge type, one of these:
// #define CHINA_MONSTER_SHIELD
#define VNH5019_DUAL_SHIELD
// #define DRV8871
// WEMOS / LOLIN V2.0.0
// #define LOLIN_I2C_MOTOR_SHIELD
// WEMOS v1.0.0
// #define WEMOS_I2C_MOTOR_SHIELD

// esp32-playmobil lok:
// #define MOTOR_START 80
// vng5019
#define MOTOR_START 40

// speaker/amplifier connected to internal DAC
// mit Arduino -> Tools -> ES32 Sketch Data Upload die F*.raw files raufladen
// #define HAVE_SOUND

// comment-out to disable
// espduino, esp32-minikit
#define INFO_LED_PIN 2
// red led on esp32-cam
// #define INFO_LED_PIN 33

// flashes when soft-ap initializes '1'
// GPIO4 = flash led beim esp32-cam
//#define RESET_INFO_PIN 4
#define RESET_INFO_PIN 2

// pwm 30% esp32-cam flash light gets too hot otherwise
// playmobil: #define HEADLIGHT_PIN 4
// putzlok:
#define HEADLIGHT_1_PIN 15
#define HEADLIGHT_1_PWM_DUTY 20

#define HEADLIGHT_2_PIN 16
#define HEADLIGHT_2_PWM_DUTY 20

// F3 == putzmotor, geht nur mit VNH5019, immer motor #1 (am board als 2 bezeichnet)
#define PUTZLOK

#define PUTZLOK_BLINK_1_PIN 32
#define PUTZLOK_BLINK_2_PIN 33
