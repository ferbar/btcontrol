#include <Button2.h>
#include "utils.h"

// on_off => callback wird beim starten einmal aufgerufen, sind derzeit nur die switches, damit wird der status festgelegt
enum when_t { on_off, on, off};
enum action_t { sendFunc, sendFullStop, direction };
struct buttonConfig_t {
  when_t when;
  int gpio;
  action_t action;
  int funcNr;
};

class Button2Config : public Button2 {
	public:
		Button2Config(buttonConfig_t &buttonConfig) : Button2(buttonConfig.gpio), buttonConfig(buttonConfig) { };
		buttonConfig_t &buttonConfig;

		static void onClick(Button2 &b) {
			Button2Config &button2Config=(Button2Config &)b;
			switch (button2Config.buttonConfig.action) {
				case sendFunc: {
					DEBUGF("cb func %d #######################\n", button2Config.buttonConfig.gpio);
					if(controlClientThread.isRunning()) {
						FBTCtlMessage cmd(messageTypeID("SETFUNC"));
						cmd["addr"]=lokdef[selectedAddrIndex].addr;
						cmd["funcnr"]=button2Config.buttonConfig.funcNr;
						cmd["value"]=b.isPressed();
						controlClientThread.query(cmd,[](FBTCtlMessage &reply) {} );
					}
					break; }
				case sendFullStop: {
					DEBUGF("cb fullstop %d #######################\n", button2Config.buttonConfig.gpio);
					sendSpeed(SPEED_STOP);
					break; }
				case direction: {
					DEBUGF("cb direction %d #######################\n", button2Config.buttonConfig.gpio);
					dirSwitch=b.isPressed() ? 1 : -1 ;
					break; }
			}
		};
};
