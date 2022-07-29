#include <assert.h>
#include <stdlib.h>
#ifdef HAVE_ALSA
#include "sound.h"
#endif
#include "utils.h"
#include "USBPlatine.h"
#include "lokdef.h"

#define TAG "USBPlatine"

USBPlatine::USBPlatine(bool debug) : debug(debug)
{
#ifndef ESP_PLATFORM
  // I2C ADC initen:
	std::string voltageFile = config.get("powerMonitor.voltage.file");
	if(voltageFile != NOT_SET) {
		std::string initFile = config.get("powerMonitor.init.file");
		std::string initValue = config.get("powerMonitor.init.value");
		if(initFile != NOT_SET) {
			writeFile(initFile, initValue);
		}
		std::string min = config.get("powerMonitor.voltage.min");
		if(min != NOT_SET) {
			this->powerMonitoringVoltageMin=stoi(min);
		}
		std::string max = config.get("powerMonitor.voltage.max");
		if(max != NOT_SET) {
			this->powerMonitoringVoltageMax=stoi(max);
		}
		this->powerMonitorVoltageFile=voltageFile;
	}
#endif
}

void USBPlatine::sendLoco(int addr_index, bool emergencyStop) {
	assert(addr_index >= 0);
	DEBUGF("USBPlatine::sendLoco(%d=>%d)", addr_index, lokdef[addr_index].addr);
				int a_speed=abs(lokdef[addr_index].currspeed);

				double f_speed=a_speed;
				if(f_speed < 5) {
					f_speed=0;
				} else {
				}

				// setPWM hängt beim RaspiPWM von den Funktionen ab
				bool func[MAX_NFUNC];
				for(int j=0; j < lokdef[addr_index].nFunc; j++) {
					func[j]=lokdef[addr_index].func[j].ison;
				}
				this->setFunction(lokdef[addr_index].nFunc, func);

				this->setDir(lokdef[addr_index].currdir < 0 ? 1 : 0 );
				this->setPWM(f_speed);
				/*
				// int ia2=lokdef[addr_index].currdir < 0 ? 255 : 0; // 255 -> relais zieht an
				int ia2=0;
				printf("%d:lokdef[addr_index=%d].currspeed: %d dir: %d pwm1 val=>%d pwm2 %d (%f)\n",this->clientID,addr_index,lokdef[addr_index].currspeed,lokdef[addr_index].currdir,ia1,ia2,f_speed);
				// printf("lokdef[addr_index].currspeed=%d: ",lokdef[addr_index].currspeed);
				*/
				this->commit();

#ifdef HAVE_ALSA
				clientFahrSound.startPlayFuncSound();
		/*
				if(lokdef[addr_index].func[1].ison && (clientFahrSound.funcSound[CFG_FUNC_SOUND_HORN] != "" )) {
					lokdef[addr_index].func[1].ison=false;
					PlayAsync horn(CFG_FUNC_SOUND_HORN);
					/ *
					Sound horn;
					horn.init(SND_PCM_NONBLOCK);
					// horn.setBlocking(false);
					horn.playSingleSound(CFG_FUNC_SOUND_HORN);
					// horn.close(false);
					* /
				}
				if(lokdef[addr_index].func[2].ison && cfg_funcSound[CFG_FUNC_SOUND_ABFAHRT] != "") {
					lokdef[addr_index].func[2].ison=false;
					PlayAsync horn(CFG_FUNC_SOUND_ABFAHRT);
				}
		*/
#endif
}

int USBPlatine::sendPOM(int addr, int cv, int value) {

#ifdef HAVE_ALSA
	if(cv == CV_CV_SOUND_VOL) {
		return CV_SOUND_VOL;
	}
	if(cv==CV_SOUND_VOL) {
		Sound::setMasterVolume(value);
		return 1;
	}
#endif
#ifndef ESP32
	if(this->powerMonitorVoltageFile != "") {
		if(cv == CV_CV_BAT) {
			return CV_BAT;
		}
		if(cv == CV_BAT) {
			std::string voltage=readFile(this->powerMonitorVoltageFile);
			int i = stoi(voltage);
			DEBUGF("BAT Voltage raw value: %d",i);
			return (i - this->powerMonitoringVoltageMin) * 255 / (this->powerMonitoringVoltageMax - this->powerMonitoringVoltageMin);
		}	
	}
#endif
	return -1;
}

int USBPlatine::sendPOMBit(int addr, int cv, int bitNr, bool value) {
	return 0;
}

void USBPlatine::clientConnected() {
#ifdef HAVE_ALSA
	clientFahrSound.startPlayFuncSound();
#endif
}

void USBPlatine::clientDisconnected(bool all) {
#ifdef HAVE_ALSA
	if(all) {
		clientFahrSound.cancel();
	}
#endif
}
