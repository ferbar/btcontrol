// example von https://github.com/Xinyuan-LilyGO/TTGO-T-Display
// https://github.com/Bodmer/TFT_eSPI  => Setup siehe https://github.com/Xinyuan-LilyGO/TTGO-T-Display !!!!!!!!!!!
#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include <Button2.h>
#include "esp_adc_cal.h"
#include "bmp.h"
#include "config.h"
#include "utils.h"
#include "lokdef.h"
#include "GuiView.h"
#include "message_layout.h"

#define TAG "btcontrol"
bool cfg_debug=false;


#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif

/*
#define TFT_MOSI            19
#define TFT_SCLK            18
#define TFT_CS              5
#define TFT_DC              16
#define TFT_RST             23
*/

#define TFT_BL              4   // Display backlight control pin
#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34


TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library
Button2 btn1(BUTTON_1); // left button
Button2 btn2(BUTTON_2); // right











long btn1Event=0;
long btn2Event=0;

char buff[512];
int vref = 1100;
int btnCick = false;

void wifi_scan();

//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void espDelay(int ms)
{
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

void showVoltage()
{
    static uint64_t timeStamp = 0;
    if (millis() - timeStamp > 1000) {
        timeStamp = millis();
        uint16_t v = analogRead(ADC_PIN);
        float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
        String voltage = "Voltage :" + String(battery_voltage) + "V";
        // Serial.println(voltage);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        //tft.fillScreen(TFT_BLACK);
        //tft.setTextDatum(TL_DATUM);
        // tft.drawString(voltage,  tft.width() / 2, tft.height() / 2 );
        tft.drawString(voltage,  0, tft.height() - 2*16 );
    }
}







void button_init_control_mode()
{
  /*
    btn1.setLongClickHandler([](Button2 & b) { // Button2.h : LONGCLICK_MS auf 1000 ändern
      Serial.println("long click handler btn1");
        btnCick = false;
        int r = digitalRead(TFT_BL);
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Press again to wake up",  tft.width() / 2, tft.height() / 2 );
        espDelay(6000);
        digitalWrite(TFT_BL, !r);

        Serial.println("TFT_DISPOFF");
        tft.writecommand(TFT_DISPOFF);
        tft.writecommand(TFT_SLPIN);
        //After using light sleep, you need to disable timer wake, because here use external IO port to wake up
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
        // esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);
        delay(200);
        esp_deep_sleep_start();
    });
    */

    /*
    btn1.setPressedHandler([](Button2 & b) {
        Serial.println("pressed handler btn1");
        btn1Event=millis();
        sendSpeed(SPEED_ACCEL);
    });
    btn1.setReleasedHandler([](Button2 & b) {
        Serial.println("released handler btn1");  
    });
    
    btn2.setPressedHandler([](Button2 & b) {
        Serial.println("pressed handler btn2");
        btn2Event=millis();
        sendSpeed(SPEED_BRAKE);
    });
*/
    
    tft.setTextDatum(BL_DATUM);
    tft.drawString("-", 0, tft.height());
    tft.setTextDatum(BR_DATUM);
    tft.drawString("+", tft.width(), tft.height());
    tft.fillScreen(TFT_BLACK);



}

void button_loop()
{
    btn1.loop();
    btn2.loop();
}


void setup()
{
    Serial.begin(115200);
    Serial.println("Start");

    /*
    ADC_EN is the ADC detection enable port
    If the USB port is used for power supply, it is turned on by default.
    If it is powered by battery, it needs to be set to high level
    */
    pinMode(ADC_EN, OUTPUT);
    digitalWrite(ADC_EN, HIGH);

    
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(0, 0);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);

    if (TFT_BL > 0) {                           // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
        pinMode(TFT_BL, OUTPUT);                // Set backlight pin to output mode
        digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
    }

    tft.setSwapBytes(true);
    tft.pushImage(0, 0,  240, 135, bootlogo);
    espDelay(5000);


    tft.setRotation(1);
    /*
    tft.fillScreen(TFT_RED);
    espDelay(1000);
    tft.fillScreen(TFT_BLUE);
    espDelay(1000);
    tft.fillScreen(TFT_GREEN);
    espDelay(1000);
*/

    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize((adc_unit_t)ADC_UNIT_1, (adc_atten_t)ADC1_CHANNEL_6, (adc_bits_width_t)ADC_WIDTH_BIT_12, 1100, &adc_chars);
    //Check type of calibration value used to characterize ADC
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
        vref = adc_chars.vref;
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
    } else {
        Serial.println("Default Vref: 1100mV");
    }


    tft.fillScreen(TFT_BLACK);


    tft.setTextDatum(TL_DATUM);

/*
    tft.drawString("LeftButton:", tft.width() / 2, tft.height() / 2 - 16);
    tft.drawString("[WiFi Scan]", tft.width() / 2, tft.height() / 2 );
    tft.drawString("RightButton:", tft.width() / 2, tft.height() / 2 + 16);
    tft.drawString("[Voltage Monitor]", tft.width() / 2, tft.height() / 2 + 32 );
    tft.drawString("RightButtonLongPress:", tft.width() / 2, tft.height() / 2 + 48);
    tft.drawString("[Deep Sleep]", tft.width() / 2, tft.height() / 2 + 64 );
    tft.setTextDatum(TL_DATUM);
*/

/*
    WiFi.begin(wifi_ssid, wifi_password);
    Serial.printf("connecting to wifi %s...\r\n", wifi_ssid);
    */
    if (!MDNS.begin(device_name)) {
        DEBUGF("Error setting up MDNS responder!");
        while(1) {
            delay(1000);
        }
    }
    Serial.println("loading message layouts");
    messageLayouts.load();

/*
#warning fixme: test
    DEBUGF("button 3 pull up");
    // macht die Button2 lib schon: gpio_set_pull_mode(GPIO_NUM_12, GPIO_PULLUP_ONLY);
    btn3.setPressedHandler([](Button2 & b) {
      DEBUGF("button 3 pressed");
    });
    */
    
/*
    // switch:
    pinMode(13, INPUT_PULLUP); */
/* ************ rotary input 
    pinMode(13, INPUT_PULLUP);
    // pinMode(13, INPUT);
    // digitalWrite(13, HIGH);       // turn on pull-up resistor
    pinMode(15, INPUT_PULLUP);
    // pinMode(15, INPUT);
    // digitalWrite(15, HIGH);       // turn on pull-up resistor
//attachInterrupt(13, isrGPIO13, CHANGE);
 attachInterrupt(13, isrGPIO13, RISING);
//attachInterrupt(15, isrGPIO13, RISING);

//attachInterrupt(15, isrGPIO15, CHANGE);
// attachInterrupt(15, isrGPIO15fall, FALLING);
*/
}


int gui_connect_state=0;
int gui_selected_loco=0;

void loop()
{
  try {
    GuiView::runLoop();
  
    // unsigned long now = millis();
 


    showVoltage();
    button_loop();

      // DEBUGF("13: %lu %lu, 15: %lu %lu", isr13rise, isr13fall, isr15rise, isr15fall);
      // DEBUGF("value:%d up down %d, up up %d, down down %d down up %d", rotaryValue, change13_up_15_down, change13_up_15_up, change13_down_15_down, change13_down_15_up);
      
      
    } catch(const char *e) {
        ERRORF(ANSI_RED "?: exception %s - client thread killed\n" ANSI_DEFAULT, e);
    } catch(std::RuntimeExceptionWithBacktrace &e) {
        ERRORF(ANSI_RED "?: Runtime Exception %s - client thread killed\n" ANSI_DEFAULT, e.what());
    } catch(std::exception &e) {
        ERRORF(ANSI_RED "?: exception %s - client thread killed\n" ANSI_DEFAULT, e.what());
        /*
    } catch (abi::__forced_unwind&) { // http://gcc.gnu.org/bugzilla/show_bug.cgi?id=28145
        ERRORF(ANSI_RED "?: forced unwind exception - client thread killed\n" ANSI_DEFAULT);
        // copy &paste:
        // printf("%d:client exit\n",startupData->clientID);
        // pthread_cleanup_pop(true);
        throw; // rethrow exeption bis zum pthread_create, dort isses dann aus
        */
    }
          
}
