#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "GuiView.h"
#include "config.h"
#include "utils.h"
#include "Button2Data.h"
#include "ControlClientThread.h"
#include "lokdef.h"
#include "remoteLokdef.h"


#define TAG "GuiView"

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

// ============================================================= GuiView ==========================
GuiView *GuiView::currGuiView=NULL;
void GuiView::loop() {
	DEBUGF("%s::loop()", this->which());
	delay(50); // ::loop() nicht überschrieben...
}

void GuiView::startGuiView(GuiView *newGuiView) {
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


// ============================================================= Wifi =============================
int GuiViewSelectWifi::selectedWifi=0;
bool GuiViewSelectWifi::needUpdate=false;
std::vector <GuiViewSelectWifi::wifiEntry_t> GuiViewSelectWifi::wifiList;

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
	if(this->selectedWifi > n-1) {
		this->selectedWifi = n-1;
	}
    // WiFi.mode(WIFI_OFF); => dürfte wifi hin machen
    DEBUGF("WiFi scan done found %d networks", n);
    btn1.setClickHandler(guiViewSelectWifiCallback1);
    btn2.setClickHandler(guiViewSelectWifiCallback2);
    btn1.setLongClickHandler(guiViewSelectWifiLongPressedCallback);
    this->needUpdate=true;
}
  
void GuiViewSelectWifi::close() {
	DEBUGF("GuiViewSelectWifi::close()");
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

const char *GuiViewSelectWifi::passwordForSSID(const String &ssid) {
	for(int i=0; i < wifiConfigSize; i++) {
		if(ssid==wifiConfig[i].ssid) {
			return wifiConfig[i].password;
		}
	}
	return NULL;
}

void GuiViewSelectWifi::loop() {
	// DEBUGF("GuiViewSelectWifi::loop()");
	// update only if changed:
	static long last=millis();
	if(this->needUpdate) {
		// DEBUGF("GuiViewSelectWifi::loop() needUpdate");
		this->needUpdate=false;
		last=millis();

		tft.fillScreen(TFT_BLACK);

		tft.setTextColor(TFT_BLACK, TFT_WHITE);
		tft.setTextDatum(TR_DATUM);
		tft.drawString(" ^",tft.width(),0);
		tft.drawString(">>",tft.width(),tft.fontHeight());
		tft.setTextDatum(BR_DATUM);
		tft.drawString(" v",tft.width(),tft.height());

		tft.setTextColor(TFT_GREEN, TFT_BLACK);
		tft.setTextDatum(MC_DATUM);
		tft.setTextSize(1);

		if (this->wifiList.size() == 0) {
			tft.drawString("No wifi networks found", tft.width() / 2, tft.height() / 2);
		} else {
			tft.setTextDatum(TL_DATUM);
			tft.setCursor(0, 0);
			DEBUGF("Found %d wifi networks", this->wifiList.size());
			int n=0;
			char buff[50];
			for(const auto& value: this->wifiList) {
				const char *password=this->passwordForSSID(value.ssid);
				DEBUGF("  wifi network %s have password %s", value.ssid.c_str(), password ? password : "no");
				int foregroundColor = this->passwordForSSID(value.ssid) ? TFT_GREEN : TFT_RED;
				int backgroundColor = TFT_BLACK;
				if(n==this->selectedWifi) {
					backgroundColor=foregroundColor;
					foregroundColor=TFT_BLACK;
				}
				tft.setTextColor(foregroundColor, backgroundColor);
				snprintf(buff,sizeof(buff),
						"%s (%ld)",
						value.ssid.c_str(),
						value.rssi);
				tft.println(buff);
				n++;
			}
		}
	} else {

		if(millis() > last+10*1000) { // alle 10 sekunden refresehen wenn keine taste gedrückt wurde
			DEBUGF("GuiViewSelectWifi::loop - refresh wifi");
			last=millis();
			GuiView::startGuiView(new GuiViewSelectWifi());
		}
	}
}

void GuiViewSelectWifi::buttonCallback(Button2 &b, int which) {
	DEBUGF("GuiViewSelectWifi::buttonCallback %d", which);
    if(which==1) {
      if(GuiViewSelectWifi::selectedWifi > 0) GuiViewSelectWifi::selectedWifi--;
    } else {
      if(GuiViewSelectWifi::selectedWifi < GuiViewSelectWifi::wifiList.size() -1) GuiViewSelectWifi::selectedWifi++;
    }
    GuiViewSelectWifi::needUpdate=true;
}
void GuiViewSelectWifi::buttonCallbackLongPress(Button2 &b) {
	DEBUGF("GuiViewSelectWifi::buttonCallbackLongPress");
	String ssid=GuiViewSelectWifi::wifiList.at(GuiViewSelectWifi::selectedWifi).ssid;
	const char *password=GuiViewSelectWifi::passwordForSSID(ssid);
    if(password) {
		GuiView::startGuiView(new GuiViewConnect(ssid, password));
    }
}


// ============================================================= Connect ==========================
void GuiViewConnect::init() {
	btn1.setClickHandler([](Button2&b) {
		// back button
		GuiView::startGuiView(new GuiViewSelectWifi());
	}
	);
	DEBUGF("GuiViewConnect::init() connecting to wifi %s %s", this->ssid.c_str(), this->password);
	this->lastWifiStatus=0;
	WiFi.mode(WIFI_STA);
	// WiFi.enableSTA(true);
	WiFi.begin(this->ssid.c_str(), this->password);
}
void GuiViewConnect::close() {
	DEBUGF("GuiViewConnect::close()");
    btn1.reset();
    btn2.reset();
}

void GuiViewConnect::loop() {
	DEBUGF("GuiViewConnect::loop() %d %d", WiFi.status(), WiFi.getMode());
	if(WiFi.status() != this->lastWifiStatus) {
		tft.setTextColor(TFT_GREEN, TFT_BLACK);
		tft.fillScreen(TFT_BLACK);
		tft.setTextDatum(MC_DATUM);
		tft.setTextSize(1);

		this->lastWifiStatus=WiFi.status();
		//		static long last=millis();
		//		refresh if status changed
		//			if(millis() > last+1000) { // jede sekunde einmal checken:
		//				last=millis();

		// grad mitn wlan verbunden:
		if(this->lastWifiStatus == WL_CONNECTED) {
			if(controlClientThread.isRunning() ) {
				tft.drawString("client thread running!!!", tft.width() / 2, tft.height() / 2);
				ERRORF("we connected to a wifi, controlClientThread should not be running");
			} else {
				IPAddress IP = WiFi.localIP();
				tft.drawString(String("connected to: ") + this->ssid + " " + IP, 0, 0 );
				int nrOfServices = MDNS.queryService("btcontrol", "tcp");

				if (nrOfServices == 0) {
					tft.println("No MDNS services were found.");
				} else {
					tft.println(String("Found ")+nrOfServices+" services");
					for (int i = 0; i < nrOfServices; i=i+1) {
						tft.println(String("Hostname: ") + MDNS.hostname(i) + "IP address: " + MDNS.IP(i) + "Port: " + MDNS.port(i));
					}
					if( nrOfServices == 1) {
						DEBUGF("============= connecting to %s:%d", String(MDNS.IP(0)).c_str(), MDNS.port(0));
						GuiView::startGuiView(new GuiViewControl(MDNS.IP(0), MDNS.port(0)));
					}

				}

			}
		} else {
			tft.setTextColor(TFT_RED, TFT_BLACK);
			tft.drawString(String("connecting to wifi ") + this->ssid + "...", 0, 0 );
			tft.setTextColor(TFT_GREEN, TFT_BLACK);
			DEBUGF("waiting for wifi %s", this->ssid.c_str());
		}
	} else {
		// DEBUGF("wifi connected");
	}
	
}

// ============================================================= Control ==========================
int GuiViewControl::selectedAddrIndex=0;
int GuiViewControl::nLokdef=0;

IPAddress GuiViewControl::host;
int GuiViewControl::port;


void GuiViewControl::init() {
	DEBUGF("GuiViewControl::init()");
	try {
		assert(!controlClientThread.isRunning());
		controlClientThread.connect(0, host, port);
		controlClientThread.start();
	} catch (std::runtime_error &e) {
		DEBUGF("error connecting / starting client thread");
		GuiView::startGuiView(new GuiViewErrorMessage(e.what()));
		return;
	}

	FBTCtlMessage cmd(messageTypeID("GETLOCOS"));
	controlClientThread.query(cmd,[this](FBTCtlMessage &reply) {
		DEBUGF("GuiViewControl::init() GETLOCOS_REPLY");
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

void GuiViewControl::close() {
	DEBUGF("GuiViewControl::close()");
}
// ============================================================= GuiViewContolLocoSelectLoco ======
bool GuiViewContolLocoSelectLoco::needUpdate=false;

void GuiViewContolLocoSelectLoco::init() {
	DEBUGF("GuiViewContolLocoSelectLoco::init() nLokdef=%d", this->nLokdef);
	GuiViewContolLocoSelectLoco::selectedAddrIndex=0;
	if(this->nLokdef==1) {
		GuiView::startGuiView(new GuiViewControlLoco());
		return;
	}
    btn1.setClickHandler([](Button2& b) {
		if(GuiViewContolLocoSelectLoco::selectedAddrIndex > 0) {
			GuiViewContolLocoSelectLoco::selectedAddrIndex--;
    		GuiViewContolLocoSelectLoco::needUpdate=true;
		}
	}
	);
    btn2.setClickHandler([](Button2& b) {
		if(selectedAddrIndex < nLokdef  -1) {
			selectedAddrIndex++;
    		needUpdate=true;
		}
	}
	);
    btn1.setLongClickHandler([](Button2& b) {
		if(nLokdef > 0) {
			GuiView::startGuiView(new GuiViewControlLoco());
		} else {
			ERRORF("no locos");
		}
	}
	);
}

void GuiViewContolLocoSelectLoco::close() {
	DEBUGF("GuiViewContolLocoSelectLoco::close()");
    btn1.reset();
    btn2.reset();
}

// Vorsicht !!! in der theorie kann lokdef beim starten da noch nicht initialisiert sein weil die callback func noch nicht aufgerufen wurde.
void GuiViewContolLocoSelectLoco::loop() {
	if(this->needUpdate) {
		tft.fillScreen(TFT_BLACK);
		DEBUGF("GuiViewContolLocoSelectLoco::loop needUpdate");
		int n=0;
		while(lokdef[n].addr) {
			if(nLokdef==this->selectedAddrIndex) {
				tft.setTextColor(TFT_BLACK, TFT_GREEN);
			} else {
				tft.setTextColor(TFT_GREEN, TFT_BLACK);
			}
			tft.drawString(String(" ") + lokdef[n].name, 0, (n+3) *16 );
			n++;
		}
	}
}

// ============================================================= ControlLoco ======================
bool GuiViewControlLoco::forceStop=false;
#define SPEED_ACCEL 10
#define SPEED_BRAKE 11
#define SPEED_STOP  12
#define SPEED_DIR_FORWARD   14          // sendet nicht wenn queue voll => resend machen
#define SPEED_DIR_BACK      15
#define SPEED_FULLSTOP      16
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
		default:
			throw std::runtime_error("sendSpeed invalid what");
	}
	if(controlClientThread.getQueueLength() > 1 && ! force) {
		DEBUGF("command in queue, dropping %s", cmdType);
		droppedCommands++;
		return;
	}
	FBTCtlMessage cmd(messageTypeID(cmdType));
	cmd["addr"]=lokdef[this->selectedAddrIndex].addr;
	if(what == SPEED_DIR_FORWARD || what == SPEED_DIR_BACK) {
		cmd["dir"]= what == SPEED_DIR_FORWARD ? 1 : -1;
	}
	controlClientThread.query(cmd,[](FBTCtlMessage &reply) {} );
}

void GuiViewControlLoco::onClick(Button2 &b) {
	DEBUGF("GuiViewControlLoco::onClick(Button2 &b)");
	if(String(currGuiView->which()) != "GuiViewControlLoco") {
		ERRORF("GuiViewControlLoco::onClick_static no GuiViewControlLoco");
		return;
	}
	GuiViewControlLoco *g=(GuiViewControlLoco *)(currGuiView);
	int selectedAddrIndex = g->selectedAddrIndex;

	Button2Data<buttonConfig_t &> &button2Config=(Button2Data<buttonConfig_t &> &)b;
	switch (button2Config.data.action) {
		case sendFunc: {
			DEBUGF("cb func %d #######################", button2Config.data.gpio);
			if(controlClientThread.isRunning()) {
				FBTCtlMessage cmd(messageTypeID("SETFUNC"));
				cmd["addr"]=lokdef[selectedAddrIndex].addr;
				cmd["funcnr"]=button2Config.data.funcNr;
				cmd["value"]=b.isPressed();
				controlClientThread.query(cmd,[](FBTCtlMessage &reply) {} );
			}
			break; }
		case sendFullStop: {
			DEBUGF("cb fullstop %d #######################", button2Config.data.gpio);
			g->forceStop=true;
			g->sendSpeed(SPEED_STOP);
			break; }
		case direction: {
			DEBUGF("cb direction %d #######################", button2Config.data.gpio);
			dirSwitch=b.isPressed() ? 1 : -1 ;
			break; }
	}
	// DEBUGF("GuiViewControlLoco::onClick done");
}

void GuiViewControlLoco::init() {
	DEBUGF("GuiViewControlLoco::init()");
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
	btn1.setLongClickHandler([](Button2&b) {
		DEBUGF("GuiViewControlLoco::btn1.setLongClickHandler");
		// off
		GuiView::startGuiView(new GuiViewPowerDown());
	}
	);
}
void GuiViewControlLoco::close() {
	DEBUGF("GuiViewControlLoco::close()");
	for(int i=0; i < buttonConfigSize; i++) {
		delete(buttons[i]);
		buttons[i]=NULL;
	}
	btn1.reset();
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

			tft.setTextColor(TFT_BLACK, TFT_WHITE);
			tft.setTextDatum(TR_DATUM);
			//tft.drawString(" ^",tft.width(),0);
			tft.drawString("off",tft.width(),tft.fontHeight());
			tft.setTextDatum(BR_DATUM);
			//tft.drawString(" v",tft.width(),tft.height());

          tft.setTextColor(TFT_GREEN, TFT_BLACK);
          tft.setTextDatum(TL_DATUM);
          if(WiFi.status() == WL_CONNECTED) {
            tft.drawString(String("AP: ") + WiFi.SSID(), 0, 0 );
          } else {
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.drawString(String("!!!: ") + WiFi.SSID(), 0, 0 );
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
          }

          tft.drawString(String("Lok: ") + lokdef[this->selectedAddrIndex].name + "  ", 0, 16*1);
          tft.drawString(String("Speed: ") + lokdef[this->selectedAddrIndex].currspeed + "  ", 0, 16*2);
          tft.drawString(lokdef[this->selectedAddrIndex].currdir > 0 ? ">" : "<", tft.width()/2, 16*2);

          tft.drawString(String("drop: ") + droppedCommands, tft.width()*3/4, 16*2);

          // ############# speed-bar:
          int color=TFT_GREEN;
          if(lokdef[selectedAddrIndex].currspeed > avg+5 || lokdef[this->selectedAddrIndex].currspeed < avg-5)
            color=TFT_RED;
          if(this->forceStop)
            color=TFT_YELLOW;
          tft.drawFastHLine(0, 50, tft.width(), color);
          int width=tft.width()* (long) lokdef[this->selectedAddrIndex].currspeed/255;
          tft.fillRect(0, 51, width, 10, color);
          tft.fillRect(width, 51, tft.width() - width, 10, TFT_BLACK);
          width=tft.width()* (long) avg/255;
          static int lastWidth=0;
          tft.drawChar(width - 5, 51+10,'^', TFT_RED, TFT_BLACK, 2);
          // DEBUGF("******************** lastWidth:%d, width:%d",lastWidth,width);
          if(lastWidth < width) {
            int w = width - lastWidth;
            if(w > 0)
              tft.fillRect(lastWidth - 5, 51+10, w, 8, TFT_BLACK);
          }
          if(lastWidth > width) {
            int w = lastWidth - width;
            // DEBUGF("******************** w: %d", w);
            if(w > 0)
              tft.fillRect(width + 5, 51+10, w, 8, TFT_BLACK);
          }
          lastWidth=width;

		  //char buf[100];
		  //snprintf(buf,sizeof(buf), "ping: ~%4.2g (%4.2g)  ", ((float)controlClientThread.pingAvg)/controlClientThread.pingCount/1000.0, controlClientThread.pingMax/1000.0);
          //tft.drawString( buf,

          tft.drawString( utils::format( "ping: ~%4.2g (%4.2g)  ", ((float)controlClientThread.pingAvg)/controlClientThread.pingCount/1000.0, controlClientThread.pingMax/1000.0).c_str(),
          	0, tft.height() - 16 );

        }
		/*
        if(btn1.isPressed() && now > btn1Event + 250) {
            btn1Event=now;
            sendSpeed(SPEED_ACCEL);
        }
        if(btn2.isPressed() && now > btn2Event + 250) {
            btn2Event=now;
            sendSpeed(SPEED_BRAKE);
        }
		*/
        static int lastValues[10]={0,0,0,0,0,0,0,0};
        static long lastPotiCheck=0;
        int poti=(analogRead(POTI_PIN)*255.0 /4095.0);

        if(now > lastPotiCheck + 200) {
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
            if(avg >= lokdef[selectedAddrIndex].currspeed + 5 && lokdef[selectedAddrIndex].currspeed <= 255-5) {
              sendSpeed(SPEED_ACCEL);
              /*
              FBTCtlMessage cmd(messageTypeID("ACC"));
              cmd["addr"]=lokdef[selectedAddrIndex].addr;
              controlClientThread.query(cmd,[](FBTCtlMessage &reply) {} );
              lokdef[selectedAddrIndex].currspeed+=5;
              */
            }
            if(avg <= lokdef[selectedAddrIndex].currspeed - 5 && lokdef[selectedAddrIndex].currspeed >= 5) {
              sendSpeed(SPEED_BRAKE);
              /*
              FBTCtlMessage cmd(messageTypeID("BREAK"));
              cmd["addr"]=lokdef[selectedAddrIndex].addr;
              controlClientThread.query(cmd,[](FBTCtlMessage &reply) {} );
              lokdef[selectedAddrIndex].currspeed-=5;
              */
            }
          }
          if(dirSwitch != lokdef[selectedAddrIndex].currdir) {
            DEBUGF("================== sending new dir ================== %d", dirSwitch);
            if(abs(lokdef[selectedAddrIndex].currspeed) > 1) {
              this->forceStop=true;
              sendSpeed(SPEED_STOP);
              /*
              FBTCtlMessage cmd(messageTypeID("STOP"));
              cmd["addr"]=lokdef[selectedAddrIndex].addr;
              controlClientThread.query(cmd,[](FBTCtlMessage &reply) {} );
              */
            } else {
              sendSpeed(dirSwitch > 0 ? SPEED_DIR_FORWARD : SPEED_DIR_BACK);
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
	}
}

// ============================================================= GuiViewErrorMessage ======================
int GuiViewErrorMessage::retries=0;

void GuiViewErrorMessage::init() {
	DEBUGF("GuiViewErrorMessage::init message:%s", this->errormessage.c_str());
}

void GuiViewErrorMessage::loop() {
	static long last=0;
	if(this->needUpdate) {
		this->needUpdate=false;
		tft.fillScreen(TFT_BLACK);
		tft.setTextDatum(MC_DATUM);
		tft.setTextSize(1);

/*
	tft.drawString("No wifi networks found", tft.width() / 2, tft.height() / 2);
	tft.setTextDatum(TL_DATUM);
	tft.setCursor(0, 0);
	*/
		tft.println(this->errormessage);
		last=millis();
	}
	if(millis() > last + 10*1000) { // 10 sekunden
		last=millis();
		DEBUGF("GuiViewErrorMessage::loop() restarting...");
		if(WiFi.status() == WL_CONNECTED) {
			retries++;
			if(retries < 5) {
				GuiView::startGuiView(new GuiViewControl());
			} else {
				retries=0;
				GuiView::startGuiView(new GuiViewSelectWifi() );
			}
		}
	}
};

// ============================================================= PowerDown ========================
void GuiViewPowerDown::init() {
	tft.fillScreen(TFT_BLACK);
	tft.setTextDatum(MC_DATUM);
	tft.setTextSize(2);
	tft.println("power off - unplug batt!!!");
}

void GuiViewPowerDown::loop() {
	if(millis() >  this->startTime + 10*1000 && ! this->done) { // 10 sekunden
		this->done=true;
		DEBUGF("GuiViewPowerDown::loop() power down...");
        // digitalWrite(TFT_BL, !r);
		digitalWrite(TFT_BL, ! TFT_BACKLIGHT_ON);

        DEBUGF("TFT_DISPOFF");
        tft.writecommand(TFT_DISPOFF);
        tft.writecommand(TFT_SLPIN);
		// esp_sleep_disable_wakeup_source(esp_sleep_disable_wakeup_source); => is im espDelay 
		delay(200);
		esp_deep_sleep_start();
	}
}
