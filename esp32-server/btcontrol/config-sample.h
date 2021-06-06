#define lok_name "esp32-playmobil"
#if false
// dev-mode, connect to your AP
// password for wifi client
  #define wifi_ssid "your-ap"
  #define wifi_password "password"
#else
// password for AP
  #define wifi_ssid lok_name
  #define wifi_password "playmobil"

  // SOFTAP => create own wifi
  #define SOFTAP
#endif

// enable update OTA
#define OTA_UPDATE

// bridge type, one of these:
// #define CHINA_MONSTER_SHIELD
// #define KEYES_SHIELD
// #define DRV8871
// WEMOS / LOLIN V2.0.0
// #define LOLIN_I2C_MOTOR_SHIELD
// WEMOS v1.0.0
#define WEMOS_I2C_MOTOR_SHIELD

#define MOTOR_START 80

// speaker/amplifier connected to internal DAC
// mit Arduino -> Tools -> ES32 Sketch Data Upload die F*.raw files raufladen
#define HAVE_SOUND

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
#define HEADLIGHT_PIN 4
#define HEADLIGHT_PWM_DUTY 20

// F3 == putzmotor, geht nur mit VNH5019, immer motor #1 (am board als 2 bezeichnet)
#define PUTZLOK
