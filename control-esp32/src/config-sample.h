#define device_name "esp32-btcontrol"
// all available ssids + password
#define WIFI_CONFIG { \
  { .ssid="ap2", .password="PASSWORD" },            \
  { .ssid="esp32-diesel", .password="btcontrol" },   \
}

// power down after XYZs inactivity
#define POWER_DOWN_IDLE_TIMEOUT 600

// enable update OTA
#define OTA_UPDATE

// comment-out to disable
// espduino, esp32-minikit
#define INFO_LED_PIN 2
// red led on esp32-cam
// #define INFO_LED_PIN 33

// flashes when soft-ap initializes '1'
// GPIO4 = flash led beim esp32-cam
//#define RESET_INFO_PIN 4
#define RESET_INFO_PIN 2

// see buttonConfig_t
#define BUTTON_CONFIG { \
  { .when=on_off, .gpio=12, .action=sendFullStop },            \
  { .when=on,     .gpio=13, .action=sendFunc, .funcNr=1},      \
  { .when=on,     .gpio=15, .action=sendFunc, .funcNr=2 },     \
  { .when=on_off, .gpio=2,  .action=direction },               \
  { .when=on_off, .gpio=17, .action=sendFunc, .funcNr=0 },     \
  { .when=on_off, .gpio=22, .action=sendFunc, .funcNr=3 },     \
}

// buttons für auf und ab
#define BUTTON_1            35
#define BUTTON_2            0
// schieberegler / poti pin
#define POTI_PIN            32

