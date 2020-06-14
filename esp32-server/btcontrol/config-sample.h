#define lok_name "esp32-playmobil"
// password for AP / wifi client
#define wifi_ssid "esp32-playmobil"
#define wifi_password "password"

// SOFTAP => create own wifi
#define SOFTAP
// enable update OTA
#define OTA_UPDATE

// bridge type, one of these:
#define CHINA_MONSTER_SHIELD
// #define KEYES_SHIELD
// #define DRV8871

// speaker/amplifier connected to internal DAC
#define HAVE_SOUND

// comment-out to disable
// espduino
//#define INFO_LED_PIN 2
// red led on esp32-cam
#define INFO_LED_PIN 33

// flashes when soft-ap initializes '1'
// GPIO4 = flash led beim esp32-cam
#define RESET_INFO_PIN 4

// pwm 30% esp32-cam flash light gets too hot otherwise
#define HEADLIGHT_PIN 4
