#include <assert.h>
#include "sound.h"
#include "utils.h"
#include "USBPlatine.h"
#include "lokdef.h"


void USBPlatine::sendLoco(int addr_index, bool emergencyStop) {
				assert(addr_index >= 0);
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
					if(cv==266) {
					    Sound::setMasterVolume(value);
						return 1;
					} else {
				#endif
						return 0;
				#ifdef HAVE_ALSA
					}
				#endif
}

int USBPlatine::sendPOMBit(int addr, int cv, int bitNr, bool value) {
	return 0;
}
