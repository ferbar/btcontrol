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
#include "ControlClientThread.h"
#include "lokdef.h"

#define TAG "btcontrol"
bool cfg_debug=false;

ControlClientThread controlClientThread;
int selectedAddrIndex=0;

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
#define BUTTON_1            35
#define BUTTON_2            0

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
        Serial.println(voltage);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        //tft.fillScreen(TFT_BLACK);
        //tft.setTextDatum(TL_DATUM);
        // tft.drawString(voltage,  tft.width() / 2, tft.height() / 2 );
        tft.drawString(voltage,  0, tft.height() - 2*16 );
    }
}

void button_init_select_loco_mode()
{
    btn1.setPressedHandler(NULL); // TODO: up
    btn2.setPressedHandler(NULL); // TODO: down
    
    tft.setTextDatum(BL_DATUM);
    tft.drawString("^", 0, tft.height());
    tft.setTextDatum(BR_DATUM);
    tft.drawString("v", tft.width(), tft.height());

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
    btn1.setPressedHandler([](Button2 & b) {
        Serial.println("pressed handler btn1");
        btn1Event=millis();
        if(controlClientThread.isRunning()) {
          FBTCtlMessage cmd(messageTypeID("ACC"));
          cmd["addr"]=3;
          controlClientThread.query(cmd,[](FBTCtlMessage &reply) {} );
        }
    });
    btn1.setReleasedHandler([](Button2 & b) {
        Serial.println("released handler btn1");  
    });
    
    btn2.setPressedHandler([](Button2 & b) {
        Serial.println("pressed handler btn2");
        btn2Event=millis();
        if(controlClientThread.isRunning()) {
          FBTCtlMessage cmd(messageTypeID("BREAK"));
          cmd["addr"]=3;
          controlClientThread.query(cmd,[](FBTCtlMessage &reply) {} );
        }      
    });
    
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

void wifi_scan()
{
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);

    tft.drawString("Scan Network", tft.width() / 2, tft.height() / 2);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    int16_t n = WiFi.scanNetworks();
    tft.fillScreen(TFT_BLACK);
    if (n == 0) {
        tft.drawString("no networks found", tft.width() / 2, tft.height() / 2);
    } else {
        tft.setTextDatum(TL_DATUM);
        tft.setCursor(0, 0);
        Serial.printf("Found %d net\n", n);
        for (int i = 0; i < n; ++i) {
            sprintf(buff,
                    "[%d]:%s(%d)",
                    i + 1,
                    WiFi.SSID(i).c_str(),
                    WiFi.RSSI(i));
            tft.println(buff);
        }
    }
    WiFi.mode(WIFI_OFF);
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


    tft.setRotation(0);
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

    WiFi.begin(wifi_ssid, wifi_password);
    Serial.printf("connecting to wifi %s...\r\n", wifi_ssid);

    if (!MDNS.begin(device_name)) {
        DEBUGF("Error setting up MDNS responder!");
        while(1) {
            delay(1000);
        }
    }
    Serial.println("loading message layouts");
    messageLayouts.load();
    
}

bool refreshLokdef=false;

void initLokdef(FBTCtlMessage reply)
{
    DEBUGF("initLokdef");
    lokdef_t *tmplokdef=lokdef;
    lokdef=NULL;
    int nLocos=reply["info"].getArraySize();
    DEBUGF("initLokdef n=%d", nLocos);
    if(lokdef) {
      tmplokdef = (lokdef_t *) realloc(tmplokdef, sizeof(lokdef_t)*(nLocos+1));
    } else {
      tmplokdef = (lokdef_t *) calloc(sizeof(lokdef_t),nLocos+1);
    }
    bzero(tmplokdef, sizeof(lokdef_t) * (nLocos+1)); // .addr = list ende
    
    
    for(int i=0; i < nLocos; i++) {
      DEBUGF("initLokdef %d\n", i);
      tmplokdef[i].currdir=0;
      tmplokdef[i].addr=reply["info"][i]["addr"].getIntVal();
      strncpy(tmplokdef[i].name, reply["info"][i]["name"].getStringVal().c_str(), sizeof(lokdef[i].name));
      strncpy(tmplokdef[i].imgname, reply["info"][i]["imgname"].getStringVal().c_str(), sizeof(lokdef[i].imgname));
      int speed=reply["info"][i]["speed"].getIntVal();
      tmplokdef[i].currspeed=abs(speed);
      tmplokdef[i].currdir=speed >=0 ? true : false;
      int functions = reply["info"][i]["functions"].getIntVal();
      for(int f=0; f < MAX_NFUNC; f++) {
        tmplokdef[i].func[f].ison=(1 >> f) | functions ? true : false;
      }

    }
    lokdef=tmplokdef;
    refreshLokdef=true;
    Serial.println("init lokdef done");
}

int gui_connect_state=0;
int gui_selected_loco=0;

void loop()
{
  try {
  static long last=millis();
  if(millis() > last+1000) { // jede sekunde einmal checken:
     last=millis();
     if(WiFi.status() != WL_CONNECTED) {
       tft.setTextColor(TFT_RED, TFT_BLACK);
       tft.drawString(String("!!!: ") + wifi_ssid, 0, 0 );
       tft.setTextColor(TFT_GREEN, TFT_BLACK);
     } else {
       if(controlClientThread.isRunning() == false) {
         IPAddress IP = WiFi.localIP();
         tft.drawString(String("connected to: ") + wifi_ssid + " " + IP, 0, 0 );
         int nrOfServices = MDNS.queryService("btcontrol", "tcp");
     
         if (nrOfServices == 0) {
           Serial.println("No services were found.");
         } else {
           Serial.print("Number of services found: ");
           Serial.println(nrOfServices);
           for (int i = 0; i < nrOfServices; i=i+1) {
             Serial.print("Hostname: ");
             Serial.println(MDNS.hostname(i));
   
             Serial.print("IP address: ");
             Serial.println(MDNS.IP(i));
   
             Serial.print("Port: ");
             Serial.println(MDNS.port(i));
   
             Serial.println("---------------");
  
           }
           if( nrOfServices == 1) {
             Serial.println("============= connecting");
             controlClientThread.connect(MDNS.IP(0), MDNS.port(0));
             controlClientThread.start();
             gui_connect_state=1;
           }
         }
       }
     }
  }
  if(controlClientThread.isRunning()) {
    // DEBUGF("gui_connect_state=%d", gui_connect_state);
    switch (gui_connect_state) {
      case 1: { // query server for locos
        FBTCtlMessage cmd(messageTypeID("GETLOCOS"));
        controlClientThread.query(cmd,[](FBTCtlMessage &reply) {
          if(reply.isType("GETLOCOS_REPLY")) {
            initLokdef(reply);
          } else {
            ERRORF("invalid reply received");
            abort();
          }
        } );
        gui_connect_state=2;
        button_init_select_loco_mode();
      }
      break;
      
      case 2: // display select dialog
        if(refreshLokdef) {
          refreshLokdef=false;
          int nLokdef=0;
          while(lokdef[nLokdef].addr) {
            if(nLokdef==gui_selected_loco) {
              tft.setTextColor(TFT_BLACK, TFT_GREEN);
            } else {
              tft.setTextColor(TFT_GREEN, TFT_BLACK);
            }
            tft.drawString(String(" ") + lokdef[nLokdef].name, 0, (nLokdef+3) *16 );
            nLokdef++;
          }
          if(nLokdef==1) { // FIXME: up down select loco + ok
            gui_connect_state=3;
            button_init_control_mode();      
            delay(1000);
            selectedAddrIndex=0;
          }
        } else {
          DEBUGF("waiting for refreshLokdef");
          delay(200);
        }
      break;
      
      case 3: // normal control mode
        if(millis() > last+200) { // jede 0,2 refreshen
          last=millis();
          tft.setTextDatum(TL_DATUM);
          tft.setTextColor(TFT_GREEN, TFT_BLACK);
          if(WiFi.status() == WL_CONNECTED) {
            tft.drawString(String("AP: ") + wifi_ssid, 0, 0 );
          } else {
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.drawString(String("!!!: ") + wifi_ssid, 0, 0 );
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
          }
          
          tft.drawString(String("Lok: ") + lokdef[selectedAddrIndex].name + " ", 0, 16*1);
          tft.drawString(String("Speed: ") + lokdef[selectedAddrIndex].currspeed + " ", 0, 16*2);
          if(btn1.isPressed() && millis() > btn1Event + 500) {
            btn1Event=millis();
            FBTCtlMessage cmd(messageTypeID("ACC"));
            cmd["addr"]=lokdef[selectedAddrIndex].addr;
            controlClientThread.query(cmd,[](FBTCtlMessage &reply) {} );
          }
          if(btn2.isPressed() && millis() > btn2Event + 500) {
            btn2Event=millis();
            FBTCtlMessage cmd(messageTypeID("BREAK"));
            cmd["addr"]=lokdef[selectedAddrIndex].addr;
            controlClientThread.query(cmd,[](FBTCtlMessage &reply) {} );
          }
        }
      break;
      default:
        ERRORF("invalid mode");
        abort();
    }
    
    
  }

      showVoltage();
      button_loop();

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
