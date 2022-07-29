#define lok_name "esp32-playmobil"
// create a softlink under data/img - maybe from bluetoothserver/img/ and upload the SPIFFS image
#define LOK_IMAGE "rangierlok.png"
// #define LOK_IMAGE "putzlok.png"

#include "config-wifi.h"
// ======= Put your wifi config in config-wifi.h:
// switch with cv #510, default is create an softap, at least one of wifi_ap* and wifi_client* is required
// password for AP
// SOFTAP => create own wifi
  #define wifi_ap_ssid lok_name
  #define wifi_ap_password "btcontrol"

// dev-mode, connect to your AP
// password for wifi client
  #define wifi_client_ssid "your-ap"
  #define wifi_client_password "your-password"

  // SOFTAP => create own wifi
  #define SOFTAP
#endif
// ======== end

// enable update OTA
#define OTA_UPDATE

// 5 für 'normale' Loks, 10 für Spiel-Loks
#define SPEED_STEP 10

// bridge type, one of these:
// #define CHINA_MONSTER_SHIELD
//#define VNH5019_DUAL_SHIELD
//#define DRV8871
// WEMOS / LOLIN V2.0.0
//#define LOLIN_I2C_MOTOR_SHIELD
// WEMOS v1.0.0
#define WEMOS_I2C_MOTOR_SHIELD

// esp32-playmobil lok: @3S /12V
#define MOTOR_START 30
// vng5019
//#define MOTOR_START 40

// speaker/amplifier connected to internal DAC
// mit Arduino -> Tools -> ES32 Sketch Data Upload die F*.raw files raufladen
#define HAVE_SOUND

// comment-out to disable, toggle bei jedem command
// espduino, esp32-minikit GPIO2 == onboard blue led
#define INFO_LED_PIN 2
// red led on esp32-cam
// #define INFO_LED_PIN 33

// flashes when soft-ap initializes '1'
// GPIO4 = flash led beim esp32-cam
// playmobil lok: headlight rear
#define RESET_INFO_PIN 4
//#define RESET_INFO_PIN 2

// pwm 30% esp32-cam flash light gets too hot otherwise
// Playmobil Lok:
#define HEADLIGHT_1_PIN 0
#define HEADLIGHT_1_PWM_DUTY 0
#define HEADLIGHT_1_PWM_DUTY_OFF 255

#define HEADLIGHT_2_PIN 4
#define HEADLIGHT_2_PWM_DUTY 0
#define HEADLIGHT_2_PWM_DUTY_OFF 255


// #define HEADLIGHT_1_PIN 15
// #define HEADLIGHT_1_PWM_DUTY 20

// #define HEADLIGHT_2_PIN 16
// #define HEADLIGHT_2_PWM_DUTY 20

// F3 == putzmotor, geht nur mit VNH5019, immer motor #1 (am board als 2 bezeichnet)
// #define PUTZLOK

// #define PUTZLOK_BLINK_1_PIN 32
// #define PUTZLOK_BLINK_2_PIN 33


/* rsyslog config:
  https://github.com/rsyslog/rsyslog-doc/blob/v8.33.1/source/configuration/templates.rst
  
$ModLoad imudp.so
$UDPServerRun 514
# security off für bunt
$EscapeControlCharactersOnReceive off

$template TraditionalFormat,"%timegenerated% %HOSTNAME% %syslogtag% %msg%\n"

if ($app-name == 'btcontrol') then {
  -/var/log/btcontrol.log;TraditionalFormat
  stop
}
*/
//#define SYSLOG_SERVER "some-syslog-server"

// Playmobil Lok Spannungsteiler 47/10kOhm
#define BAT_ADC_PIN1 32
// 11V => 2130
#define BAT_ADC_MIN 2000
// 12,5V
#define BAT_ADC_MAX 2463


