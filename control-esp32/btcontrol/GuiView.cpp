#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "GuiView.h"
#include "config.h"
#include "utils.h"
#include "Button2Config.h"

#define TAG "GuiView"

// is im btcontrol
extern TFT_eSPI tft;
extern Button2 btn1;
extern Button2 btn2;

GuiViewSelectWifi::wifiConfigEntry_t wifiConfig[] = WIFI_CONFIG ;
const int wifiConfigSize=sizeof(wifiConfig) / sizeof(wifiConfig[0]);



buttonConfig_t buttonConfig[] = BUTTON_CONFIG;

const int buttonConfigSize=sizeof(buttonConfig)/sizeof(buttonConfig[0]);

Button2Config *buttons[buttonConfigSize];

// ============================================================= GuiView ==========================
void GuiView::loop() {
    GuiView::startGuiView(GuiViewSelectWifi());
  };
  
void GuiView::startGuiView(const GuiView &newGuiView) {
    DEBUGF("startGuiView current: %s new: %s", currGuiView.which(), newGuiView.which());
    currGuiView.close();
    currGuiView=newGuiView;
    currGuiView.init();
}

void GuiView::runLoop() {
    currGuiView.loop();
}


// ============================================================= Wifi =============================
int GuiViewSelectWifi::selectedWifi=0;
bool GuiViewSelectWifi::needUpdate=false;
void guiViewSelectWifiCallback1(Button2 &b);
void guiViewSelectWifiCallback2(Button2 &b);
void guiViewSelectWifiLongPressedCallback(Button2 &b);



void GuiViewSelectWifi::init() {    

    DEBUGF("WiFi Mode STA");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    int16_t n = WiFi.scanNetworks();
    this->wifiList.clear();
    for(int i=0; i < n; i++) {
      this->wifiList.push_back( { .ssid=WiFi.SSID(i), .rssi=WiFi.RSSI(i) } );
    }
    WiFi.mode(WIFI_OFF);
    DEBUGF("WiFi scan done found %d networks", n);
    btn1.setClickHandler(guiViewSelectWifiCallback1);
    btn2.setClickHandler(guiViewSelectWifiCallback2);
    btn1.setLongClickHandler(guiViewSelectWifiLongPressedCallback);
    this->needUpdate=true;
}
  
void GuiViewSelectWifi::close() {
    this->wifiList.clear();
    btn1.reset();
    btn2.reset();
}
  
void guiViewSelectWifiLongPressedCallback(Button2 &b) {
  GuiViewSelectWifi::buttonCallbackLongPress(b);
}
void guiViewSelectWifiCallback1(Button2 &b) {
  GuiViewSelectWifi::buttonCallback(b,1);
}
void guiViewSelectWifiCallback2(Button2 &b) {
  GuiViewSelectWifi::buttonCallback(b,2);
}

bool GuiViewSelectWifi::havePasswordForSSID(const String &ssid) {
	for(int i=0; i < wifiConfigSize; i++) {
		if(ssid==wifiConfig[i].password) {
			return true;
		}
	}
	return false;
}

void GuiViewSelectWifi::loop() {
    if(this->needUpdate) {
      this->needUpdate=false;
      // update only if changed:
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.fillScreen(TFT_BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setTextSize(1);

      if (this->wifiList.size() == 0) {
        tft.drawString("no networks found", tft.width() / 2, tft.height() / 2);
      } else {
        tft.setTextDatum(TL_DATUM);
        tft.setCursor(0, 0);
        Serial.printf("Found %d wifi networks\n", this->wifiList.size());
        int n=0;
		char buff[50];
        for(const auto& value: this->wifiList) {
          int foregroundColor = this->havePasswordForSSID(value.ssid) ? TFT_GREEN : TFT_RED;
          int backgroundColor = TFT_BLACK;
          if(n==this->selectedWifi) {
            backgroundColor=foregroundColor;
            foregroundColor=TFT_BLACK;
          }
          tft.setTextColor(foregroundColor, backgroundColor);
          snprintf(buff,sizeof(buff),
                    "%s(%ld)",
                    value.ssid.c_str(),
                    value.rssi);
          tft.println(buff);
        }
      }
    }
    
}

void GuiViewSelectWifi::buttonCallback(Button2 &b, int which) {
    if(which==1) {
      if(GuiViewSelectWifi::selectedWifi > 0) GuiViewSelectWifi::selectedWifi--;
    } else {
      if(GuiViewSelectWifi::selectedWifi < GuiViewSelectWifi::wifiList.size() -1) GuiViewSelectWifi::selectedWifi++;
    }
    GuiViewSelectWifi::needUpdate=true;
}
void GuiViewSelectWifi::buttonCallbackLongPress(Button2 &b) {
	String ssid=GuiViewSelectWifi::wifiList.at(GuiViewSelectWifi::selectedWifi).ssid;
    if(GuiViewSelectWifi::havePasswordForSSID(ssid)) {
      GuiView::startGuiView(GuiViewConnect(ssid));
    }
}


// ============================================================= Connect ==========================
void GuiViewConnect::init() {
	btn1.setClickHandler([](Button2&b) {
		GuiView::startGuiView(GuiViewSelectWifi());
	}
	);
}
void GuiViewConnect::close() {
    btn1.reset();
    btn2.reset();
}

void GuiViewConnect::loop() {
	if(WiFi.status() != this->lastWifiStatus) {
		int oldStatus=this->lastWifiStatus;
		this->lastWifiStatus=WiFi.status();
		//		static long last=millis();
		//		refresh if status changed
		//			if(millis() > last+1000) { // jede sekunde einmal checken:
		//				last=millis();

		// grad mitn wlan verbunden:
		if(this->lastWifiStatus == WL_CONNECTED) {
			if(controlClientThread.isRunning() ) {
				ERRORF("we connected to a wifi, controlClientThread should not be running");
			} else {
				IPAddress IP = WiFi.localIP();
				tft.drawString(String("connected to: ") + this->ssid + " " + IP, 0, 0 );
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
						GuiView::startGuiView(GuiViewControlLoco());
					}

				}

			}
		} else {
			tft.setTextColor(TFT_RED, TFT_BLACK);
			tft.drawString(String("!!!: ") + this->ssid, 0, 0 );
			tft.setTextColor(TFT_GREEN, TFT_BLACK);
			DEBUGF("waiting for wifi %s", this->ssid.c_str());
		}
	} else {
		// DEBUGF("wifi connected");
	}
	
}

void GuiViewControllLoco::init() {
	 for(int i=0; i < buttonConfigSize; i++) {
        buttons[i]=new Button2Config(buttonConfig[i]);

        if(buttonConfig[i].when == on_off || buttonConfig[i].when == on) {
            Serial.printf("set %i on handler\n", i);
            buttons[i]->setPressedHandler(Button2Config::onClick);
        }
        if(buttonConfig[i].when == on_off || buttonConfig[i].when == off) {
            Serial.printf("set %i off handler\n", i);
            buttons[i]->setReleasedHandler(Button2Config::onClick);
        }
        // init senden. on_off haben nur die switches derzeit
        if(buttonConfig[i].when == on_off) {
            Button2Config::onClick(*buttons[i]);
        }
    }
  }
void GuiViewControllLoco::close() {
	for(int i=0; i < buttonConfigSize; i++) {
		delete(buttons[i]);
		buttons[i]=NULL;
	}
}
void GuiViewControllLoco::loop() {
    for(int i=0; i < buttonConfigSize; i++) {
        if(buttons[i]) {
          buttons[i]->loop();
        } else {
          // Serial.printf("Button[%d] not initialized\n", i);
        }
    }
  }



