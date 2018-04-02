/**
 * PWM über den Raspberry PI
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "RaspiPWM.h"
#include "utils.h"
#include <stdexcept>
#include <exception>
#include <assert.h>
#include <wiringPi.h>
#include <softPwm.h>

#include "ParseExpr.h"

int cfg_pinPWM=1;

ParseExpr *parseExpr=NULL;

RaspiPWM::RaspiPWM(bool debug) :
		USBPlatine(debug),
		dir(-1), pwm(-1), motorStart(70), motorFullSpeed(255), motorFullSpeedBoost(255),
		fRaspiLed(NULL), raspiLedToggle(0), nFunc(0),
		funcThread(*this) {

	assert(parseExpr==NULL); // nur einmal da?
	parseExpr=new ParseExpr();

	this->currentFunc[0]=1; // F0 beim booten ein
	for(int i=1; i < MAX_NFUNC; i++) {
		this->currentFunc[i]=0;
	}

	try {
		this->init();
	} catch (std::exception &e) {
		// printf("RaspiPWM::RaspiPWM Exception: %s\n", e.what());
		this->release();
		throw ; // rethrow
	}
}

RaspiPWM::~RaspiPWM() {
	this->fullstop();
	this->release();
	delete(parseExpr); parseExpr=NULL;
	this->funcThread.cancel();
	printf("RaspiPWM::~RaspiPWM() done\n");
}

void RaspiPWM::release() {
}

void RaspiPWM::init() {

	printf("RaspiPWM::init()\n");
	wiringPiSetupGpio();

	std::string tmp = config.get("digispark.motorStart");
	this->motorStart=utils::stoi(tmp);
	tmp = config.get("digispark.motorFullSpeed");
	if(tmp == NOT_SET) {
		//Default
	} else {
		this->motorFullSpeed=utils::stoi(tmp);
	}
	printf("RaspiPWM::init() ---- motorStart %d, fullspeed %d\n", this->motorStart, this->motorFullSpeed);

	tmp = config.get("digispark.motorFullSpeedBoost");
	if(tmp == NOT_SET) {
		//Default
		this->motorFullSpeedBoost = this->motorStart; // nix eingestellt
	} else {
		this->motorFullSpeedBoost=utils::stoi(tmp);
		printf("RaspiPWM::init() ---- motorFullSpeedBoost %d\n", this->motorFullSpeedBoost);
	}

	bool F0=this->currentFunc[0];
	for (auto it=config.begin(); it!=config.end(); ++it) {
		if(utils::startsWith(it->first,"wiringpi.pin.")) {
			int pin = stoi(it->first.substr(strlen("wiringpi.pin.")));
			if(("wiringpi.pin." + std::to_string(pin) == it->first) || ("wiringpi.pin." + std::to_string(pin) + ".a" == it->first)) {
				printf("setting wiringpi.pin[%d]=%s\n", pin, it->second.c_str());
				if(it->second == "pwm" ) {
					cfg_pinPWM=pin;
					printf("PWM on pin %d\n", pin);
					continue;
				}
				// test ob parsbar
				parseExpr->getResult(it->second, 0, 0, F0);
				// pin mode initialisieren
				pinMode(pin, OUTPUT);
				// digitalWrite(pin, value);
				int pwm=100;
				std::string conf_pwm=config.get(it->first + ".pwm");
				if(conf_pwm != NOT_SET) {
					pwm=stoi(conf_pwm);
					printf("RaspiPWM::init() ---- softPwm pin %d=>%d\n",pin,pwm);
					if(pwm < 0 || pwm > 100) {
						printf("error: only 0-100 allowed\n");
						abort();
					}
				}
				this->pins.insert(PinCtl::pair(pin,{.function=it->second, .lastState=false, .pwm=pwm}));
				// this->pins[pin]=new PinCtl(it->second);
			}
		}
	}	
	if(pins.size() == 0) {
		printf("WARN: no pin definitions found\n");
	}

	this->fRaspiLed = fopen("/sys/class/leds/led0/brightness", "w");
	if(this->fRaspiLed) {
		printf("can write to Raspberry ACT led1\n");
	} else {
		printf("error opening raspi ACT led (%s)\n",strerror(errno));
	}

	pinMode(cfg_pinPWM, PWM_OUTPUT);
	pwmSetMode(PWM_MODE_MS);
	pwmSetClock(4);
	pwmSetRange(256);
	this->setPWM(0);
	this->setDir(0); // default dir = 0
	// test ob parsbar + bits initialisieren
	this->commit(true);
	this->dumpPins();
	this->funcThread.start();
}

void RaspiPWM::setPWM(int f_speed) {
	// Umrechnen in PWM einheiten
	// 255 = pwm max
	unsigned char pwm = 0;
	if(f_speed > 0) {
		if(f_speed == 255 && this->currentFunc[9]) { // motor FullSpeed ignorieren
			pwm = this->motorFullSpeedBoost;
		} else {
			pwm = f_speed*((double)this->motorFullSpeed - this->motorStart)/255 + this->motorStart;
		}
	}
	// int result = 0;
	if(this->pwm!=pwm) {
		printf("setting pwm: %d\n", pwm);
		pwmWrite(cfg_pinPWM, pwm);
		this->pwm=pwm;
	}

	if(this->fRaspiLed) {
		if(this->raspiLedToggle & 0x1)
			fwrite(this->raspiLedToggle & 0x2 ? "1\n" : "0\n", 1, 2, this->fRaspiLed); fflush(this->fRaspiLed);
		this->raspiLedToggle++;
	}
}

void RaspiPWM::setDir(unsigned char dir) {
	// int result = 0;
	if(this->dir!=dir) {
		printf("setting dir: %d\n", dir);
		/*
		for(int i=0; i<this->maxPins ; i++) {
			if(this->pinsDir1[i] >= 0)
				digitalWrite(this->pinsDir1[i], dir==1 ? 0 : 1);
			if(this->pinsDir2[i] >= 0)
				digitalWrite(this->pinsDir2[i], dir==1 ? 1 : 0);
		}
		*/
		this->dir=dir;
	}
}

void RaspiPWM::setFunction(int nFunc, bool *func) {
	memcpy(&this->currentFunc, func, sizeof(this->currentFunc));
}

void RaspiPWM::fullstop() {
	this->setPWM(0);
	printf("RaspiPWM::fullstop() done\n");
}

void RaspiPWM::dumpPins() {
	printf("RaspiPWM::dumpPins()\n");
	for(const auto &element: this->pins) {
		printf("%d: %s (%d)\n", element.first, element.second.function.c_str(), element.second.pwm);
	}
}

void RaspiPWM::commit() {
	this->commit(false);
}

void RaspiPWM::commit(bool force) {
this->dumpPins();
	Lock lock(this->funcMutex);
	bool F0=this->currentFunc[0];
	auto it=this->pins.begin();
	while (it!=this->pins.end()) {
		int pin=it->first;
		printf("%d\n", pin);
		PinCtl *pinCtl=&it->second;
		bool value=false;
		do {
			if(parseExpr->getResult(it->second.function, this->dir, this->pwm, F0)) {
				printf("   %s [true, last:%d]\n", it->second.function.c_str(), it->second.lastState);
				value=true;
				pinCtl=&it->second;
			} else {
				printf("   %s [false, last:%d]\n", it->second.function.c_str(), it->second.lastState);
				if(it->second.lastState) {
					if(it->second.pwm != 100) {
						printf("  soft pwm stop\n");
						softPwmStop(it->first);
					} else {
						digitalWrite(pin, value);
					}
					it->second.lastState=false;
				}
			}
			++it;
		} while((it != this->pins.end() ) && (it->first == pin));
		printf("%d: %s =>%d (pwm:%d last:%d)\n", pin, pinCtl->function.c_str(),value,pinCtl->pwm,pinCtl->lastState);
		if(force || value != pinCtl->lastState) {
			if(pinCtl->pwm != 100) {
				if(value) {
					printf("  soft pwm create\n");
					softPwmCreate(pin, pinCtl->pwm, 100);
				}
			} else {
				digitalWrite(pin, value);
			}
			pinCtl->lastState=value;
		} else {
			printf("  no force\n");
		}
	}
}

void RaspiPWMFuncThread::run() {
	while(true) {
		printf("RaspiPWMFuncThread::run()\n");
		raspiPWM.commit(false);
		sleep(1);
	}
}
