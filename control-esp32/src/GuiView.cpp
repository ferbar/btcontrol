#include <TFT_eSPI.h>
// #include <Fonts/GFXFF/FreeSans9pt7b.h>
// #define FONT9PT &FreeSans9pt7bBitmaps
/*
#define FSB9 &FreeSerifBold9pt7b
#define FSB12 &FreeSerifBold12pt7b
#define FSB18 &FreeSerifBold18pt7b
#define FSB24 &FreeSerifBold24pt7b
#define FSS9 &FreeSans9pt7b
#define FSS12 &FreeSans12pt7b
#define FSS18 &FreeSans18pt7b
#define FSS24 &FreeSans24pt7b
#include "fonts/sourcesanspro5pt7b.h"
#define FSSP5 &SourceSansPro_Regular5pt7b
#include "fonts/sourcesanspro6pt7b.h"
#define FSSP6 &SourceSansPro_Regular6pt7b */
#include "fonts/sourcesanspro7pt7b.h"
#define FSSP7 &SourceSansPro_Regular7pt7b

#include <SPI.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>

#include "GuiView.h"
#include "config.h"
#include "utils.h"
#include "Button2Data.h"
#include "ControlClientThread.h"
#include "lokdef.h"
#include "remoteLokdef.h"


#define TAG "GuiView"

// PNG Functions
#include "TFT_eSPI_PNG.h"


// is im btcontrol
extern TFT_eSPI tft;
extern Button2 btn1;
extern Button2 btn2;

GuiViewSelectWifi::wifiConfigEntry_t wifiConfig[] = WIFI_CONFIG ;
// const int wifiConfigSize=sizeof(wifiConfig) / sizeof(wifiConfig[0]);
const int wifiConfigSize=countof(wifiConfig);



// on_off => callback wird beim starten einmal aufgerufen, sind derzeit nur die switches, damit wird der status festgelegt
enum when_t { on_off, on, off};
enum action_t { sendFunc, sendFullStop, direction };
struct buttonConfig_t {
  when_t when;
  int gpio;
  action_t action;
  int funcNr;
};


buttonConfig_t buttonConfig[] = BUTTON_CONFIG;

// const int buttonConfigSize=sizeof(buttonConfig)/sizeof(buttonConfig[0]);
const int buttonConfigSize=countof(buttonConfig);

Button2Data <buttonConfig_t&> *buttons[buttonConfigSize];


ControlClientThread controlClientThread;

// ^
uint8_t arrowBitmap[]= {
  0,128,
  1,128+64,
  1+2,128+64+32,
  1+2+4,128+64+32+16,
  1+2+4+8,128+64+32+16+8,
  1+2+4+8+16,128+64+32+16+8+4,
  1+2+4+8+16+32,128+64+32+16+8+4+2,
  1+2+4+8+16+32+64,128+64+32+16+8+4+2+1
};

// reset callback handler
void resetButtons() {
  btn1.setChangedHandler(NULL);
  btn1.setPressedHandler(NULL);
  btn1.setReleasedHandler(NULL);
  btn1.setTapHandler(NULL);
  btn1.setClickHandler(NULL);
  btn1.setDoubleClickHandler(NULL);
  btn1.setTripleClickHandler(NULL);
  btn1.setLongClickHandler(NULL);
  btn1.setLongClickDetectedHandler(NULL);

  btn2.setChangedHandler(NULL);
  btn2.setPressedHandler(NULL);
  btn2.setReleasedHandler(NULL);
  btn2.setTapHandler(NULL);
  btn2.setClickHandler(NULL);
  btn2.setDoubleClickHandler(NULL);
  btn2.setTripleClickHandler(NULL);
  btn2.setLongClickHandler(NULL);
  btn2.setLongClickDetectedHandler(NULL);
}

// ============================================================= GuiView ==========================
long GuiView::lastKeyPressed=0;
GuiView *GuiView::currGuiView=NULL;
void GuiView::loop() {
	DEBUGF("%s::loop()", this->which());
	delay(50); // ::loop() nicht überschrieben...
}

void GuiView::startGuiView(GuiView *newGuiView) {
  // DEBUGF("GuiView::startGuiView");
  // utils::dumpBacktrace();
  if(currGuiView) {
    DEBUGF("startGuiView current: %s new: %s", currGuiView->which(), newGuiView->which());
    currGuiView->close();
    delete(currGuiView);
  } else {
    DEBUGF("startGuiView new: %s", newGuiView->which());
  }
  currGuiView=newGuiView;
  currGuiView->init();
}

void GuiView::runLoop() {
	if(currGuiView) {
		// DEBUGF("GuiView::runLoop() %s", currGuiView->which());
    	currGuiView->loop();
	} else {
		DEBUGF("no currGuiView");
	}
}

void GuiView::drawButtons() {
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(NULL);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setTextDatum(TR_DATUM);
  tft.drawString(" ^", tft.width(), 0);
  tft.drawString(">>", tft.width(), tft.fontHeight());
  tft.setTextDatum(BR_DATUM);
  tft.drawString(" v", tft.width(), tft.height());
  tft.drawString("off", tft.width(), tft.height() - tft.fontHeight());

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(1);
  tft.setFreeFont(FSSP7);
  tft.setTextDatum(TL_DATUM);
     /*
      tft.setCursor(0, tft.fontHeight()); // freeFont malt immer mit baseline
     */
}

// ============================================================= SelectWifi =============================
int GuiViewSelectWifi::selectedWifi=0;
bool GuiViewSelectWifi::needUpdate=false;

std::map <String, GuiViewSelectWifi::WifiEntry> GuiViewSelectWifi::wifiList;

void guiViewSelectWifiCallback1(Button2 &b);
void guiViewSelectWifiCallback2(Button2 &b);
void guiViewSelectWifiLongPressedCallback(Button2 &b);



void GuiViewSelectWifi::init() {    

  DEBUGF("WiFi Mode STA");
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(1);
  tft.setFreeFont(FSSP7);
  // 20210922 setTextDatum(MC) scheint die 0 pos vom string in die mitte vom string zu setzen. Koordinaten 0/0 sind trozdem links oben
  tft.drawString("Scanning...", tft.width()/2, tft.height()/2);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  if(esp_wifi_set_protocol( WIFI_IF_STA, WIFI_PROTOCOL_11B| WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR ) != ESP_OK ) {
    tft.drawString("  esp_wifi_set_protocol failed", 0, tft.fontHeight()*2);
  }
  WiFi.disconnect();
  delay(100);

  int16_t n = WiFi.scanNetworks();
  this->wifiList.clear();
  for(int i=0; i < n; i++) {
    wifi_ap_record_t *scanResult=(wifi_ap_record_t *)WiFi.getScanInfoByIndex(i);
    NOTICEF("  - found wifi %s rssi:%d, chan:%d, %d %d %d %d", scanResult->ssid, scanResult->rssi, scanResult->primary, scanResult->phy_11b, scanResult->phy_11g, scanResult->phy_11n, scanResult->phy_lr);

    auto it = this->wifiList.find(WiFi.SSID(i));
    if(it == this->wifiList.end()) {
      this->wifiList[WiFi.SSID(i)] = { scanResult->rssi, scanResult->phy_lr? true : false };
    } else {
      DEBUGF("  found duplicate SSID: %s (old:%d, new:%d)", WiFi.SSID(i).c_str(), it->second.rssi, WiFi.RSSI(i));
      // multiple APs for the same ssid found, use the lower rssi;
      if(this->wifiList[WiFi.SSID(i)].rssi < WiFi.RSSI(i)) // rssi is always negative!! -40 is better than -90!
        this->wifiList[WiFi.SSID(i)].rssi = WiFi.RSSI(i) ;
    }
  }
	if(this->selectedWifi > n-1) {
		this->selectedWifi = n-1;
	}
  // WiFi.mode(WIFI_OFF); => dürfte wifi hin machen
  DEBUGF("WiFi scan done found %d networks", n);
  // don't start scan if btn1 pressed
  btn1.setPressedHandler([](Button2&b) {
    GuiViewSelectWifi::lastKeyPressed=millis();
  });
  btn1.setClickHandler(guiViewSelectWifiCallback1);
  btn2.setClickHandler(guiViewSelectWifiCallback2);
  btn1.setLongClickDetectedHandler(guiViewSelectWifiLongPressedCallback);
  btn2.setLongClickDetectedHandler([](Button2&b) {
    DEBUGF("GuiViewSelectWifi::btn2.setLongClickHandler");
    // off
    GuiView::startGuiView(new GuiViewPowerDown());
  }
  );
  btn1.setPressedHandler([](Button2&b) {
    GuiViewSelectWifi::lastKeyPressed=millis();
  });
  btn2.setPressedHandler([](Button2&b) {
    GuiViewSelectWifi::lastKeyPressed=millis();
  });
  this->needUpdate=true;
  tft.fillScreen(TFT_BLACK);
}
  
void GuiViewSelectWifi::close() {
	DEBUGF("GuiViewSelectWifi::close()");
  this->wifiList.clear();
  resetButtons();
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

const char *GuiViewSelectWifi::passwordForSSID(const String &ssid) {
	for(int i=0; i < wifiConfigSize; i++) {
		if(ssid==wifiConfig[i].ssid) {
			return wifiConfig[i].password;
		}
	}
	return NULL;
}

void GuiViewSelectWifi::loop() {
	//DEBUGF("GuiViewSelectWifi::loop()");
	// update only if changed:
	static long last=millis();       // für refresh
	if(this->needUpdate) {
		DEBUGF("##################GuiViewSelectWifi::loop() needUpdate");
		this->needUpdate=false;
		last=millis();

		this->drawButtons();   

		if (this->wifiList.size() == 0) {
      tft.setTextDatum(MC_DATUM);
			tft.drawString("No wifi networks found", tft.width() / 2, tft.height() / 2);
		} else {
			DEBUGF("Found %d wifi networks", this->wifiList.size());
      // tft.fillScreen(TFT_BLACK); text inhalt ändert sich nicht
      this->drawButtons();
			int n=0;
			char buff[50];
      for(int i=0; i < 2; i++) {
			  for(const auto& value: this->wifiList) {
				  const String password=this->passwordForSSID(value.first);
  				DEBUGF("  wifi network %s have password: %s", value.first.c_str(), password ? "yes" : "no");
          // round 1: list APs with passwords
          if((password && i==0) || (!password && i==1)) {
	  			  int foregroundColor = password ? TFT_GREEN : TFT_RED;
  		  		int backgroundColor = TFT_BLACK;
	  		  	if(n==this->selectedWifi) {
		  		  	// backgroundColor=foregroundColor;
              backgroundColor=TFT_BLUE;
  			  		// foregroundColor=TFT_BLACK;
              // foregroundColor=TFT_YELLOW;
  		  		}
	  		  	tft.setTextColor(foregroundColor, backgroundColor);
		  		  snprintf(buff,sizeof(buff),
			  			"[%d] %s (%d%s)",
              n,
					  	value.first.c_str(),
						  value.second.rssi,
						  value.second.have_LR ? " LR" : "");
  				//ft.println(buff); => println geht ned richtig mit custom fonts (baseline falsch, bg color geht ned)
            tft.drawString(buff,0,tft.fontHeight()*n);
	  	  		n++;
          }
			  }
			}
		}
	} else {
    if(millis() > GuiViewSelectWifi::lastKeyPressed+POWER_DOWN_IDLE_TIMEOUT*1000) { // nach 5min power down
      DEBUGF("GuiViewSelectWifi::loop() powerDown");
      GuiView::startGuiView(new GuiViewPowerDown());
    }

		if(millis() > last+10*1000) { // alle 10 sekunden refresehen wenn keine taste gedrückt wurde
			DEBUGF("##############GuiViewSelectWifi::loop - restart SelectWifi for rescan");
			last=millis();
			GuiView::startGuiView(new GuiViewSelectWifi());
		}
	}
}

void GuiViewSelectWifi::buttonCallback(Button2 &b, int which) {
	DEBUGF("GuiViewSelectWifi::buttonCallback %d", which);
  GuiViewSelectWifi::lastKeyPressed=millis();
  if(which==1) {
    if(GuiViewSelectWifi::selectedWifi > 0) GuiViewSelectWifi::selectedWifi--;
  } else {
    if(GuiViewSelectWifi::selectedWifi < GuiViewSelectWifi::wifiList.size() -1) GuiViewSelectWifi::selectedWifi++;
  }
  GuiViewSelectWifi::needUpdate=true;
}

void GuiViewSelectWifi::buttonCallbackLongPress(Button2 &b) {
	DEBUGF("GuiViewSelectWifi::buttonCallbackLongPress");
  GuiViewSelectWifi::lastKeyPressed=millis();
	String ssid;
  int n=0;
  bool LR=false;
	for(auto it=GuiViewSelectWifi::wifiList.begin(); it != GuiViewSelectWifi::wifiList.end(); ++it) {
    if(GuiViewSelectWifi::passwordForSSID(it->first)) {
      DEBUGF("have password for %s", it->first.c_str());
      if(n==GuiViewSelectWifi::selectedWifi) {
        ssid=it->first;
        LR=it->second.have_LR;
        break;
      }
      n++;
    }
	}
  if(ssid) {
	  const char *password=GuiViewSelectWifi::passwordForSSID(ssid);
    if(password) {
		  GuiView::startGuiView(new GuiViewConnectWifi(ssid, password, LR));
    } else {
      DEBUGF("no password for %d", GuiViewSelectWifi::selectedWifi);
    }
  } else {
    DEBUGF("no wifi selected");
  }
}

// ============================================================= GuiViewConnectWifi ==========================
int GuiViewConnectWifi::mdnsResults=0;
int GuiViewConnectWifi::selectedServer=0;
bool GuiViewConnectWifi::needUpdate=0;

void GuiViewConnectWifi::init() {
  DEBUGF("GuiViewConnectWifi::init() connecting to wifi %s %s", this->ssid.c_str(), this->password);
  /* back button?
	btn1.setClickHandler([](Button2&b) {
		// back button
		GuiView::startGuiView(new GuiViewSelectWifi());
	}
	);*/
	this->lastWifiStatus=0;
	WiFi.mode(WIFI_STA);

  if(this->LR) {
    esp_wifi_set_protocol (WIFI_IF_STA, WIFI_PROTOCOL_LR);
  } // else im scanner wird BGN LE aktiviert
	// WiFi.enableSTA(true);
	WiFi.begin(this->ssid.c_str(), this->password);
 
  if (!MDNS.begin(device_name)) {
    ERRORF("Error setting up mDNS");
    abort();
  }
  WiFi.setAutoReconnect(true);

  btn1.setClickHandler([](Button2&b) {
    if(GuiViewConnectWifi::selectedServer > 0)
      GuiViewConnectWifi::selectedServer--;
    GuiViewConnectWifi::needUpdate=true;
  });
  btn2.setClickHandler([](Button2&b) {
    if(GuiViewConnectWifi::selectedServer < (GuiViewConnectWifi::mdnsResults-1))
      GuiViewConnectWifi::selectedServer++;
    GuiViewConnectWifi::needUpdate=true;
  });
    
  btn1.setLongClickDetectedHandler([](Button2&b) {
    GuiView::startGuiView(new GuiViewConnectServer(MDNS.IP(GuiViewConnectWifi::selectedServer), MDNS.port(GuiViewConnectWifi::selectedServer)));
  } );
  btn2.setLongClickDetectedHandler([](Button2&b) {
    DEBUGF("GuiViewSelectWifi::btn2.setLongClickHandler");
    // off
    GuiView::startGuiView(new GuiViewPowerDown());
  } );
  btn1.setPressedHandler([](Button2&b) {
    GuiView::lastKeyPressed=millis();
  });
  btn2.setPressedHandler([](Button2&b) {
    GuiView::lastKeyPressed=millis();
  });
  GuiViewConnectWifi::needUpdate=true;
}

void GuiViewConnectWifi::close() {
	DEBUGF("GuiViewConnectWifi::close()");
  resetButtons();
}

void GuiViewConnectWifi::loop() {
  static long last;
	if(WiFi.status() != this->lastWifiStatus || this->needUpdate) {
    DEBUGF("GuiViewConnectWifi::loop() WifiStatus:%d (old:%d), WifiMode: %d", WiFi.status(), this->lastWifiStatus, WiFi.getMode());
		tft.setTextColor(TFT_GREEN, TFT_BLACK);
		tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
		tft.setTextSize(1);
    tft.setCursor(0, 0);

		this->lastWifiStatus=WiFi.status();
		//		static long last=millis();
		//		refresh if status changed
		//			if(millis() > last+1000) { // jede sekunde einmal checken:
		//				last=millis();

		// grad mitn wlan verbunden:
		if(this->lastWifiStatus == WL_CONNECTED) {
			if(controlClientThread.isRunning() ) {
        tft.setTextDatum(MC_DATUM);
				tft.drawString("client thread running!!!", tft.width() / 2, tft.height() / 2);
				ERRORF("we connected to a wifi, controlClientThread should not be running");
			} else {
				IPAddress ip = WiFi.localIP();
				tft.drawString(String("connected to: ") + this->ssid + " " + ip.toString(), 0, 0 );
        wifi_power_t txpower=WiFi.getTxPower();
        
        tft.drawString(String("   txpower: ") + txpower, 0, tft.fontHeight());
        uint8_t protocolBitmap;
        if(esp_wifi_get_protocol(WIFI_IF_STA, &protocolBitmap) == ESP_OK) {
          tft.drawString(String("   protocol: ") + (protocolBitmap & WIFI_PROTOCOL_11B ? "B" : "") +
            (protocolBitmap & WIFI_PROTOCOL_11G ? "G" : "") +
            (protocolBitmap & WIFI_PROTOCOL_11N ? "N" : "") +
            (protocolBitmap & WIFI_PROTOCOL_LR ? "LR" : ""), 0, tft.fontHeight() * 2);
        }

        static long lastRefresh=0;
        if(lastRefresh + 10*1000 < millis()) {
  				GuiViewConnectWifi::mdnsResults = MDNS.queryService("btcontrol", "tcp");
          lastRefresh=millis();
        }

        DEBUGF("nrOfServices: %d", GuiViewConnectWifi::mdnsResults);
				if (this->mdnsResults == 0) {
					tft.println("No MDNS services were found.");
          tft.println("Check accesspoint. MDNS works with IDF v 3.3!");
				} else {
					if( this->mdnsResults == 1) {
						DEBUGF("============= connecting to %s:%d", (MDNS.IP(0).toString()).c_str(), MDNS.port(0));
            tft.println(String("Hostname: ") + MDNS.hostname(0) + "IP address: " + MDNS.IP(0).toString() + "Port: " + MDNS.port(0));
						GuiView::startGuiView(new GuiViewConnectServer(MDNS.IP(0), MDNS.port(0)));
					} else {
            this->displaySelectServer();
					}

				}

			}
		} else {
      if(this->connectingStartedAt + 10000 < millis()) {
        DEBUGF("unable to connect within 10s...");
        GuiView::startGuiView(new GuiViewSelectWifi() );
        return;
      }
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawString(String("connecting to wifi ") + this->ssid + "...", 0, 0 );
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      DEBUGF("waiting for wifi %s", this->ssid.c_str());
		}
    GuiViewConnectWifi::needUpdate=false;
    last=millis();
	} else {
		// DEBUGF("wifi connected");
    if(millis() > last+10*1000) { // alle 10 sekunden refresehen wenn keine taste gedrückt wurde
      DEBUGF("##############GuiViewSelectWifi::loop - restart SelectWifi for rescan");
      last=millis();
      GuiViewConnectWifi::needUpdate=true;
    }
	}
	
}

void GuiViewConnectWifi::displaySelectServer() {
  DEBUGF("GuiViewConnectWifi::displaySelectServer() selected:%d", GuiViewConnectWifi::selectedServer);

  this->drawButtons();

  for(int i=0; i < this->mdnsResults; i++) {
    if(i==GuiViewConnectWifi::selectedServer) {
      tft.setTextColor(TFT_BLACK, TFT_GREEN);
    } else {
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
    }
    tft.drawString(String(" ") + MDNS.hostname(i) + " (" + MDNS.IP(i).toString() + ":" + MDNS.port(i) + ")", 0, i * tft.fontHeight() );
  }
}

// ============================================================= ConnectServer ==========================
int GuiViewConnectServer::nLokdef=0;

IPAddress GuiViewConnectServer::host;
int GuiViewConnectServer::port;


void GuiViewConnectServer::init() {
	DEBUGF("GuiViewConnectServer::init()");
	try {
		assert(!controlClientThread.isRunning());
		controlClientThread.connect(0, host, port);
		controlClientThread.start();
	} catch (std::runtime_error &e) {
		DEBUGF("error connecting / starting client thread");
		GuiView::startGuiView(new GuiViewErrorMessage(String("Error connecting to ") + host.toString() + ":\n" + e.what()));
		return;
	}

	FBTCtlMessage cmd(messageTypeID("GETLOCOS"));
	controlClientThread.query(cmd,[this](FBTCtlMessage &reply) {
		DEBUGF("GuiViewConnectServer::init() GETLOCOS_REPLY");
		if(reply.isType("GETLOCOS_REPLY")) {
			lokdef_t *orglokdef=lokdef;
			nLokdef=0;
			// !!!! race condition !!!!!
			lokdef=initLokdef(reply);
			if(orglokdef != NULL) {
				free(orglokdef);
			}
			while(lokdef[nLokdef].addr) {
				nLokdef++;
			}
			GuiView::startGuiView(new GuiViewContolLocoSelectLoco());
		} else {
			ERRORF("invalid reply received");
			abort();
		}
	}
	);
}

void GuiViewConnectServer::close() {
	DEBUGF("GuiViewConnectServer::close()");
}

void GuiViewConnectServer::loop() {
  DEBUGF("GuiViewConnectServer::loop() clientThread running: %d, wifi status: %d", controlClientThread.isRunning(), WiFi.status());
  delay(100);
}
// ============================================================= GuiViewContolLocoSelectLoco ======
bool GuiViewContolLocoSelectLoco::needUpdate=false;
int GuiViewContolLocoSelectLoco::firstLocoDisplayed=0;

void GuiViewContolLocoSelectLoco::init() {
	DEBUGF("GuiViewContolLocoSelectLoco::init()");
	controlClientThread.selectedAddrIndex=0;
	if(lokdef[0].addr && ! lokdef[1].addr) {  // only one loco in list
		GuiView::startGuiView(new GuiViewControlLoco());
		return;
	}
  // Up
  btn1.setClickHandler([](Button2& b) {
		if(controlClientThread.selectedAddrIndex > 0) {
			controlClientThread.selectedAddrIndex--;
		if(controlClientThread.selectedAddrIndex < GuiViewContolLocoSelectLoco::firstLocoDisplayed)
			GuiViewContolLocoSelectLoco::firstLocoDisplayed--;

   		GuiViewContolLocoSelectLoco::needUpdate=true;
		}
	}
	);
  // Down
  btn2.setClickHandler([](Button2& b) {
		if(lokdef[controlClientThread.selectedAddrIndex].addr && lokdef[controlClientThread.selectedAddrIndex+1].addr) {
			controlClientThread.selectedAddrIndex++;
      int fontHeight=17;
      
      DEBUGF("btn2.setClickHandler first:%d, selected: %d, fontheight:%d, height:%d",
        GuiViewContolLocoSelectLoco::firstLocoDisplayed,
        controlClientThread.selectedAddrIndex,
        fontHeight,
        tft.height());
      if((controlClientThread.selectedAddrIndex - GuiViewContolLocoSelectLoco::firstLocoDisplayed)*fontHeight >= tft.height())
        GuiViewContolLocoSelectLoco::firstLocoDisplayed++;

   		needUpdate=true;
		}
	}
	);
  // Select Loco
  btn1.setLongClickDetectedHandler([](Button2& b) {
		if(lokdef[0].addr) { // only if we have locos in list
			GuiView::startGuiView(new GuiViewControlLoco());
		} else {
			ERRORF("no locos");
		}
	}
	);
  // Power down
  btn2.setLongClickDetectedHandler([](Button2& b) {
    DEBUGF("GuiViewSelectWifi::btn2.setLongClickHandler");
    GuiView::startGuiView(new GuiViewPowerDown());
  }
  );
  tft.fillScreen(TFT_BLACK);
}

void GuiViewContolLocoSelectLoco::close() {
	DEBUGF("GuiViewContolLocoSelectLoco::close()");
  resetButtons();
}

std::map<const std::string, TFT_eSprite> imgCache;
Mutex imgCacheMutex;
/**
 * malt ein Bild wenns schon im cache ist, wenn nicht request und return ohne zu malen + GuiViewContolLocoSelectLoco::needUpdate=true;
 */
void drawCachedImage(const char*imgname, int x, int y) {
  if(strlen(imgname) == 0) {
    DEBUGF("drawCachedImage no imgname - ignored");
    return;
  }
  // DEBUGF("drawCachedImage %s", imgname);
  Lock lock(imgCacheMutex);
  auto imgPair=imgCache.find(imgname);
  if(imgPair == imgCache.end()) {
    DEBUGF("requesting image [%s]",imgname);
	  FBTCtlMessage cmd(messageTypeID("GETIMAGE"));
	  cmd["imgname"]=imgname;
	  controlClientThread.query(cmd,[imgname](FBTCtlMessage &reply) { 
      if(reply.isType("GETIMAGE_REPLY")) {
        // reply.dump();
        Lock lock(imgCacheMutex);
        std::string data=reply["img"].getStringVal();
        NOTICEF("got getimage reply for %s - length:%dB",imgname, data.length());
        std::pair<const std::string, TFT_eSprite> pair(imgname, TFT_eSprite(&tft));
        imgCache.insert(pair); // das sollte gehen weil zu dem Zeitpunkt spr kopiert werden kann (pointer alle null)
        auto it=imgCache.find(imgname);
        TFT_eSprite &spr=it->second;
        // imgPair.second;
        if(data.length() > 5000) {
          ERRORF("image %s too big (%d)",imgname, data.length());
          spr.createSprite(0,0);
          return;
        }
        long startms=millis();
        // TFT_eSprite spr = TFT_eSprite(&tft);
        TFT_eSPI_PngSprite png(spr);
        png.setPngPosition(0, 0);
        png.setTransparentColor(TFT_WHITE);
        spr.createSprite(20,20);
        png.load_data(data);
        DEBUGF("image decoded in %ldms",millis()-startms);

        GuiViewContolLocoSelectLoco::needUpdate=true;
      } else {
        ERRORF("didn't receive getimage_reply");
      }
      } 
    );
    return; // beim ersten run nur request aufs img machen, img nicht anzeigen
  }
  /*
  if(imgCache[imgname] == "invalid") {
    DEBUGF("image invalid");
    return;
  }
  */
  // long startms=millis();
  // NOTICEF("drawing image @%d:%d",x,y);
  TFT_eSprite &spr=imgPair->second;
  spr.pushSprite(x,y);
  // DEBUGF("rendered image in %ldms", millis()-startms);
}

// Vorsicht !!! in der theorie kann lokdef beim starten da noch nicht initialisiert sein weil die callback func noch nicht aufgerufen wurde.
void GuiViewContolLocoSelectLoco::loop() {

	if(this->needUpdate) {
    this->drawButtons();
		DEBUGF("GuiViewContolLocoSelectLoco::loop needUpdate - firstLoco:%d fontheight:%d", this->firstLocoDisplayed, tft.fontHeight());
		int n=0;
    // -20 damit wir die up down buttons nicht auch noch löschen
		tft.setTextPadding(tft.width()-18-20);
		while(lokdef[n].addr) {
			if(n==controlClientThread.selectedAddrIndex) {
				tft.setTextColor(TFT_BLACK, TFT_GREEN);
			} else {
				tft.setTextColor(TFT_GREEN, TFT_BLACK);
			}
			int sumx=tft.drawString(String(" ") + lokdef[n].name+"", 18 , (n-this->firstLocoDisplayed) * tft.fontHeight() );
      // bug: sumx stimmt nicht ganz
      sumx-=1;
      // clear rest:
      // -20 damit wir die up down buttons nicht auch noch löschen
      // NOTICEF("[%d] draw loco sumx:%d", n, sumx);
      //tft.fillRect(18+sumx, (n-this->firstLocoDisplayed) * tft.fontHeight(), tft.width()-sumx-18-20, tft.fontHeight(), TFT_BLACK /* n%2 ? TFT_RED : TFT_CYAN */);
      drawCachedImage(lokdef[n].imgname, 0, (n-this->firstLocoDisplayed) * tft.fontHeight() );
			n++;
		}
    tft.setTextPadding(0);
    this->needUpdate=false;
	}
}

// ============================================================= ControlLoco ======================
bool GuiViewControlLoco::forceStop=false;
#define SPEED_ACCEL 10
#define SPEED_BRAKE 11
void sendSpeed(int what);
int droppedCommands=0;

// 1 oder -1
int dirSwitch=1;


void GuiViewControlLoco::sendSpeed(int what) {
	if(!controlClientThread.isRunning()) {
		DEBUGF("sendSpeed - not connected");
		return;
	}
	bool force=false;
	const char *cmdType=NULL;
	switch(what) {
		case SPEED_ACCEL:
			cmdType="ACC";
			break;
		case SPEED_BRAKE:
			cmdType="BREAK";
			break;
      /*
		case SPEED_STOP:
			cmdType="STOP";
			force=true;
			break;
		case SPEED_FULLSTOP:
			cmdType="STOP";
			force=true;
			break;
      
		case SPEED_DIR_FORWARD:
			cmdType="DIR";
			break;
		case SPEED_DIR_BACK:
			cmdType="DIR";
			break;
      */
		default:
			throw std::runtime_error("sendSpeed invalid what");
	}
	if(controlClientThread.getQueueLength() > 1 && ! force) {
		DEBUGF("command in queue, dropping %s", cmdType);
		droppedCommands++;
		return;
	}
  // DEBUGF(" GuiViewControlLoco::sendSpeed ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
	FBTCtlMessage cmd(messageTypeID(cmdType));
	cmd["addr"]=controlClientThread.getCurrLok().addr;
	controlClientThread.query(cmd,[](FBTCtlMessage &reply) {} );

  GuiViewControlLoco::lastKeyPressed=millis();  // poti geändert oder notaus gedrückt => wir kommen da her
}

void GuiViewControlLoco::onClick(Button2 &b) {
	DEBUGF("GuiViewControlLoco::onClick(Button2 &b)");
	if(String(currGuiView->which()) != "GuiViewControlLoco") {
		ERRORF("GuiViewControlLoco::onClick_static no GuiViewControlLoco");
		return;
	}
	GuiViewControlLoco *g=(GuiViewControlLoco *)(currGuiView);

  // dynamic_cast wäre schöner, geht aber nicht
	Button2Data<buttonConfig_t &> &button2Config=(Button2Data<buttonConfig_t &> &) b;

	switch (button2Config.data.action) {
		case sendFunc: {
			DEBUGF("cb func %d #######################", button2Config.data.gpio);
			if(controlClientThread.isRunning()) {
        controlClientThread.sendFunc(button2Config.data.funcNr, b.isPressed() );
			}
			break; }
		case sendFullStop: {
			DEBUGF("cb fullstop %d #######################", button2Config.data.gpio);
			g->forceStop=true;
      if(controlClientThread.isRunning()) {
        controlClientThread.sendStop();
      }
			break; }
		case direction: {
			DEBUGF("cb direction %d #######################", button2Config.data.gpio);
			dirSwitch=b.isPressed() ? 1 : -1 ;
			break; }
	}
  GuiViewControlLoco::lastKeyPressed=millis();  // Taste gedrückt => wir kommen da her
	// DEBUGF("GuiViewControlLoco::onClick done");
}

void GuiViewControlLoco::init() {
	DEBUGF("GuiViewControlLoco::init()");
  GuiViewControlLoco::lastKeyPressed=millis();
	for(int i=0; i < buttonConfigSize; i++) {
		buttons[i]=new Button2Data<buttonConfig_t &>(buttonConfig[i].gpio, buttonConfig[i]);

		if(buttonConfig[i].when == on_off || buttonConfig[i].when == on) {
			DEBUGF("set %i on handler", i);
			buttons[i]->setPressedHandler(GuiViewControlLoco::onClick);
		}
		if(buttonConfig[i].when == on_off || buttonConfig[i].when == off) {
			DEBUGF("set %i off handler", i);
			buttons[i]->setReleasedHandler(GuiViewControlLoco::onClick);
		}
		// init senden. on_off haben nur die switches derzeit
		if(buttonConfig[i].when == on_off) {
			DEBUGF("call on_off handler");
			GuiViewControlLoco::onClick(*buttons[i]);
		}
	}
	tft.fillScreen(TFT_BLACK);
 	btn2.setLongClickDetectedHandler([](Button2&b) {       // erst ab Button2 Version 1.5.0
//  btn2.setLongClickHandler([](Button2&b) {
		DEBUGF("GuiViewControlLoco::btn2.setLongClickHandler");
		// off
		GuiView::startGuiView(new GuiViewPowerDown());
	}
	);
  // load functions:
  FBTCtlMessage cmd(messageTypeID("GETFUNCTIONS"));
  cmd["addr"]=controlClientThread.getCurrLok().addr;
  controlClientThread.query(cmd,[this](FBTCtlMessage &reply) {
    DEBUGF("GuiViewControlLoco::init() GETFUNCTIONS_REPLY");
    if(reply.isType("GETFUNCTIONS_REPLY")) {
      initLokdefFunctions(lokdef, controlClientThread.selectedAddrIndex, reply);
    } else {
      ERRORF("invalid reply received");
      abort();
    }
  });
}

void GuiViewControlLoco::close() {
	DEBUGF("GuiViewControlLoco::close()");
	for(int i=0; i < buttonConfigSize; i++) {
		delete(buttons[i]);
		buttons[i]=NULL;
	}
	resetButtons();
}

void GuiViewControlLoco::loop() {
	// DEBUGF("GuiViewControlLoco::loop()");
    for(int i=0; i < buttonConfigSize; i++) {
        if(buttons[i]) {
          buttons[i]->loop();
        } else {
          // Serial.printf("Button[%d] not initialized\n", i);
        }
    }
	if(controlClientThread.isRunning()) {
	  if(lokdef==NULL) {
	    ERRORF("no lokdef");
      abort();
	  }
    // DEBUGF("gui_connect_state=%d", gui_connect_state);
	  unsigned long now = millis();
	  static long last=0;
        static int avg=0;
        if(now > last+200) { // jede 0,2 refreshen
          last=millis();

          tft.setFreeFont(NULL);
          tft.setTextColor(TFT_BLACK, TFT_WHITE);
          tft.drawString("off",tft.width(), tft.height() - tft.fontHeight());
          tft.setTextDatum(TL_DATUM);
          tft.setTextColor(TFT_GREEN, TFT_BLACK);
          // ping stats:
          tft.drawString( utils::format( "ping: ~%4.2g (%4.2g) drop: %d ", ((float)controlClientThread.pingAvg)/controlClientThread.pingCount/1000.0, controlClientThread.pingMax/1000.0, droppedCommands).c_str(),
            0, tft.height() - tft.fontHeight() );

          // Func:
          //DEBUGF("func for lok %d lokdef: %p <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<", this->selectedAddrIndex, lokdef);
          if(lokdef) {
            String func;
            int x_pos=0;
            // func+=controlClientThread.getCurrLok().nFunc; func+=" ";
            for(int i=0; i < controlClientThread.getCurrLok().nFunc; i++) {
              if(controlClientThread.getCurrLok().func[i].name[0]) {
                if(controlClientThread.getCurrLok().func[i].ison) {
                  tft.setTextColor(TFT_BLACK, TFT_WHITE);
                } else {
                  tft.setTextColor(TFT_WHITE, TFT_BLACK);
                }
              
                func=(char) toupper(controlClientThread.getCurrLok().func[i].name[0]);
                x_pos+=tft.drawString(func, x_pos, tft.height() - tft.fontHeight()*2 )*2;
              }
            }
          }
          tft.setTextColor(TFT_WHITE, TFT_BLACK);


          tft.setFreeFont(FSSP7);

          if(WiFi.status() == WL_CONNECTED) {
            int powerDownSec =  (GuiViewControlLoco::lastKeyPressed+POWER_DOWN_IDLE_TIMEOUT*1000 - millis())/1000;
            static int lastWidth=0;
            int width=0;
            if(powerDownSec < 30) {
              tft.setTextColor(TFT_BLACK, TFT_RED);
              width=tft.drawString(String("Power down in ") + powerDownSec + "s", 0, 0 );
              tft.setTextColor(TFT_WHITE, TFT_BLACK);
            } else {
              if( WiFi.SSID() != controlClientThread.getCurrLok().name) {  // AP nur anzeigen wenn lokname != AP
                width=tft.drawString(String("AP: ") + WiFi.SSID(), 0, 0 );
              } else {
                width=0;
              }
            }
            if(width < lastWidth) {
              tft.fillRect(width-1, 0, lastWidth-width, tft.fontHeight(), TFT_BLUE); // TFT_BLACK);
            }
            lastWidth=width;
          } else {
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.drawString(String("!!!: ") + WiFi.SSID(), 0, 0 );
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
          }

          drawCachedImage(controlClientThread.getCurrLok().imgname, 0, tft.fontHeight() );

          tft.drawString(String() + controlClientThread.getCurrLok().name + "   ", 20, tft.fontHeight());
          tft.drawString(String("Speed: ") + controlClientThread.getCurrLok().currspeed + "    ", 0, tft.fontHeight()*2);
          tft.drawString(controlClientThread.getCurrLok().currdir > 0 ? ">" : "<", tft.width()/2, tft.fontHeight()*2);


          // ############# speed-bar:
          int color=TFT_GREEN;
          if(controlClientThread.getCurrLok().currspeed > avg+5 || controlClientThread.getCurrLok().currspeed < avg-5)
            color=TFT_RED;
          if(this->forceStop)
            color=TFT_YELLOW;
          // Line
          tft.drawFastHLine(0, 50, tft.width(), color);
          // Speed-Bar
          int width=tft.width()* (long) controlClientThread.getCurrLok().currspeed/255;
          tft.fillRect(0, 51, width, 10, color);
          tft.fillRect(width, 51, tft.width() - width, 10, TFT_BLACK);
          // ^          => fixme: braucht zuviel höhe
          width=tft.width()* (long) avg/255;
          static int lastWidth=0;
          // DEBUGF("drawing ^ @%d:%d",width-5, 51+10);
          int v_width=16;
          tft.drawBitmap(width-8,51+10,arrowBitmap,16,8,TFT_WHITE, TFT_BLACK);
          // DEBUGF("******************** lastWidth:%d, width:%d",lastWidth,width);
          if(lastWidth < width) {
            int w = width - lastWidth;
            if(w > 0)
              tft.fillRect(lastWidth - 8, 51+10, w, tft.fontHeight()*2, TFT_BLACK);
          }
          if(lastWidth > width) {
            int w = lastWidth - width;
            // DEBUGF("******************** w: %d", w);
            if(w > 0)
              tft.fillRect(width - 8 + v_width, 51+10, w, tft.fontHeight()*2, TFT_BLACK);
          }
          lastWidth=width;

        }

        static int lastValues[10]={0,0,0,0,0,0,0,0};
        static long lastPotiCheck=0;
        if(now > lastPotiCheck + 200) {
          int analog=analogRead(POTI_PIN);
          int poti=(analog*255.0 /4095.0);
          lastPotiCheck=now;
          for(int i=0; i < 9; i++) {
            lastValues[i]=lastValues[i+1];
          }
          lastValues[9]=poti;
          avg=0;
          for(int i=0; i < 10; i++) {
            // Serial.printf("[%d]=%d ", i , lastValues[i]);
            avg+=lastValues[i];
          }
          // int avg10=avg;
          avg/=10;
          // DEBUGF("#########poti value: %d, switch: %d, avg: %d, avg10: %d, currspeed=%d, currdir=%d", poti, dirSwitch, avg, avg10, lokdef[selectedAddrIndex].currspeed, lokdef[selectedAddrIndex].currdir);
          if(this->forceStop) {
            if(avg==0) {
              this->forceStop=false;
            }
          } else {
            if(avg >= controlClientThread.getCurrLok().currspeed + 5 && controlClientThread.getCurrLok().currspeed <= 255-5) {
              sendSpeed(SPEED_ACCEL);
              /*
              FBTCtlMessage cmd(messageTypeID("ACC"));
              cmd["addr"]=lokdef[selectedAddrIndex].addr;
              controlClientThread.query(cmd,[](FBTCtlMessage &reply) {} );
              lokdef[selectedAddrIndex].currspeed+=5;
              */
            }
            if(avg <= controlClientThread.getCurrLok().currspeed - 5 && controlClientThread.getCurrLok().currspeed >= 5) {
              sendSpeed(SPEED_BRAKE);
              /*
              FBTCtlMessage cmd(messageTypeID("BREAK"));
              cmd["addr"]=lokdef[selectedAddrIndex].addr;
              controlClientThread.query(cmd,[](FBTCtlMessage &reply) {} );
              lokdef[selectedAddrIndex].currspeed-=5;
              */
            }
          }
          if(dirSwitch != controlClientThread.getCurrLok().currdir) {
            DEBUGF("================== sending new dir ================== %d", dirSwitch);
            if(abs(controlClientThread.getCurrLok().currspeed) > 1) {
              this->forceStop=true;
              if(controlClientThread.isRunning()) {
                controlClientThread.sendStop();
              }
              /*
              FBTCtlMessage cmd(messageTypeID("STOP"));
              cmd["addr"]=lokdef[selectedAddrIndex].addr;
              controlClientThread.query(cmd,[](FBTCtlMessage &reply) {} );
              */
            } else {
              controlClientThread.sendDir(dirSwitch > 0);
              /*
              FBTCtlMessage cmd(messageTypeID("DIR"));
              cmd["addr"]=lokdef[selectedAddrIndex].addr;
              cmd["dir"]=dirSwitch;
              controlClientThread.query(cmd,[](FBTCtlMessage &reply) {} );
              */
            }
          }
        }

	} else { // controlClientThread.isRunning is not running
		DEBUGF("GuiViewControlLoco::loop() controlClientThread.isRunning is not running");
		GuiView::startGuiView(new GuiViewErrorMessage("lost connection ... reconnecting"));
    return;
	}
 
  if(millis() > GuiViewControlLoco::lastKeyPressed+POWER_DOWN_IDLE_TIMEOUT*1000) { // nach 5min power down
    DEBUGF("GuiViewControlLoco::loop() powerDown");
    GuiView::startGuiView(new GuiViewPowerDown());
    return;
  }
}

// ============================================================= GuiViewErrorMessage ======================
int GuiViewErrorMessage::retries=0;

void GuiViewErrorMessage::init() {
	DEBUGF("GuiViewErrorMessage::init message:\"%s\"", this->errormessage.c_str());
}

void GuiViewErrorMessage::loop() {
	static long last=0;
	if(this->needUpdate) {
		this->needUpdate=false;
		tft.fillScreen(TFT_BLACK);
		tft.setTextDatum(MC_DATUM);
		tft.setTextSize(1);
    int pos=0;
    int n=0;
    while(pos < this->errormessage.length()) {
      int oldpos=pos;
      pos=this->errormessage.indexOf("\n", pos);
      if(pos >= 0) {
        tft.drawString(this->errormessage.substring(oldpos,pos), tft.width()/2, tft.height()/2+tft.fontHeight()*n);
      } else {
        tft.drawString(this->errormessage.substring(oldpos), tft.width()/2, tft.height()/2+tft.fontHeight()*n);
        break;
      }
      pos++;
      n++;
    }
    if(WiFi.status() != WL_CONNECTED) {
      tft.drawString(String("lost wifi connection, retries=")+retries, 0, tft.height()/2+tft.fontHeight()*n);
    }
		last=millis();
	}
	if(millis() > last + 1*1000) { // 1 sekunden
		last=millis();
		DEBUGF("GuiViewErrorMessage::loop() restarting, retries=%d wifi status=%d...", retries, WiFi.status());
		if(WiFi.status() == WL_CONNECTED) {
			if(retries < 10) {
        retries++;
				GuiView::startGuiView(new GuiViewConnectServer());
        return;
			} else {
				retries=0;
				GuiView::startGuiView(new GuiViewSelectWifi() );
        return;
			}
		} else {
      if(retries >= 20) { // nach 20 sekunden zum select wifi
        retries=0;
        GuiView::startGuiView(new GuiViewSelectWifi() );
        return;
      }
		}
    retries++;
    this->needUpdate=true;
	}
};

// ============================================================= PowerDown ========================
void guiViewPowerDownBackToControl(Button2 &b) {
  DEBUGF("guiViewPowerDownBackToControl() changed: %d, pressed:%d", b.getAttachPin(), b.isPressed() );
  GuiView::startGuiView(new GuiViewControlLoco());
}

void GuiViewPowerDown::init() {
	tft.fillScreen(TFT_BLACK);
	tft.setTextDatum(MC_DATUM);
	tft.setTextSize(2);
	tft.drawString("power off", tft.width()/2,tft.height()/2);
	tft.drawString("unplug batt!!!", tft.width()/2, tft.height()/2 + tft.fontHeight());
  tft.setTextSize(1);
  DEBUGF("GuiViewPowerDown::init()");
  if(controlClientThread.isRunning()) {
    controlClientThread.sendStop();
  }

  btn1.setClickHandler(guiViewPowerDownBackToControl);
  btn2.setClickHandler(guiViewPowerDownBackToControl);
  
  for(int i=0; i < buttonConfigSize; i++) {
    assert(buttons[i]==NULL);
    DEBUGF("GuiViewPowerDown::init button %d => pin %d", i, buttonConfig[i].gpio);
    buttons[i]=new Button2Data<buttonConfig_t &>(buttonConfig[i].gpio, buttonConfig[i]);
    // bug: ist ein button vor init '1', wird initialisiert mit setChangedHandler wird der handler aufgerufen .....
    if(buttons[i]->isPressedRaw() )
      buttons[i]->setReleasedHandler(guiViewPowerDownBackToControl);
    else
      buttons[i]->setPressedHandler(guiViewPowerDownBackToControl);
  }
  GuiViewPowerDown::startTime=millis();
}

void GuiViewPowerDown::close() {
  DEBUGF("GuiViewPowerDown::close()");
  for(int i=0; i < buttonConfigSize; i++) {
    delete(buttons[i]);
    buttons[i]=NULL;
  }
}

void GuiViewPowerDown::loop() {
    // DEBUGF("GuiViewControlLoco::loop()");
    for(int i=0; i < buttonConfigSize; i++) {
        if(buttons[i]) {
          buttons[i]->loop();
        } else {
          // Serial.printf("Button[%d] not initialized\n", i);
        }
    }
	if(millis() >  GuiViewPowerDown::startTime + 10*1000 && ! this->done) { // 10 sekunden
		this->done=true;
    if(controlClientThread.isRunning()) {
      controlClientThread.cancel();
    }
		DEBUGF("GuiViewPowerDown::loop() power down ******************************************************************************");
        // digitalWrite(TFT_BL, !r);
		digitalWrite(TFT_BL, ! TFT_BACKLIGHT_ON);

    DEBUGF("TFT_DISPOFF");
    tft.writecommand(TFT_DISPOFF);
    tft.writecommand(TFT_SLPIN);
		// esp_sleep_disable_wakeup_source(esp_sleep_disable_wakeup_source); => is im espDelay 
		delay(500);
		esp_deep_sleep_start();
	}
}
