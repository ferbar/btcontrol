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
#include <wiringPi.h>
#include <assert.h>


#include "ParseExpr.h"

int cfg_pinPWM=1;

ParseExpr *parseExpr=NULL;

RaspiPWM::RaspiPWM(bool debug) :
		USBPlatine(debug),
		dir(-1), pwm(-1), motorStart(70), motorFullSpeed(255), motorFullSpeedBoost(255),
		fRaspiLed(NULL), raspiLedToggle(0), nFunc(0) {

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
			if(utils::endsWith(it->first,".softPwm")) {
				if(this->pins.find(pin) == this->pins.end())
					printf("RaspiPWM::init() ---- invaild softPwm pin\n");
					abort();
				}
				this->pins[pin].softPwm=stoi(it->second);
			} else {
				printf("setting wiringpi.pin[%d]=%s\n", pin, it->second.c_str());
				if(it->second == "pwm" ) {
					cfg_pinPWM=pin;
					printf("PWM on pin %d\n", pin);
					continue;
				}
				// test ob parsbar + bits initialisieren
				bool value=parseExpr->getResult(it->second, 0, 0, F0);
				pinMode(pin, OUTPUT);
				digitalWrite(pin, value);
				this->pins[pin]=new PinCtl(it->second);
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

void RaspiPWM::commit() {
	bool F0=this->currentFunc[0];
	for (auto it=this->pins.begin(); it!=this->pins.end(); ++it) {
		bool value=parseExpr->getResult(it->second->function, this->dir, this->pwm, F0);
		// printf("digitalWrite[%d] => %d (%s)\n",it->first,value,it->second.c_str());
		if(it->second.lastState == PinCtl.UNDEFINED || value != it->second.lastState) {
			digitalWrite(it->first, value);
			it->second->lastState=value;
		}
	}
}
