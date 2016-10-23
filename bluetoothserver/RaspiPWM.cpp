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

#define PIN_PWM		1

ParseExpr *parseExpr=NULL;

RaspiPWM::RaspiPWM(bool debug) :
	USBPlatine(debug),
	 dir(-1), pwm(-1), motorStart(70), motorFullSpeed(255),
		fRaspiLed(NULL), raspiLedToggle(0), nFunc(0) {

	assert(parseExpr==NULL); // nur einmal da?
	parseExpr=new ParseExpr();


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
	wiringPiSetup();


	std::string tmp = config.get("digispark.motorStart");
	this->motorStart=utils::stoi(tmp);
	tmp = config.get("digispark.motorFullSpeed");
	if(tmp == NOT_SET) {
		//Default
	} else {
		this->motorFullSpeed=utils::stoi(tmp);
	}
	printf("RaspiPWM::init() ---- motorStart %d, fullspeed %d\n", this->motorStart, this->motorFullSpeed);

	for (auto it=config.begin(); it!=config.end(); ++it) {
		if(utils::startsWith(it->first,"wiringpi.pin.")) {
			int pin = stoi(it->first.substr(strlen("wiringpi.pin.")));
			printf("setting wiringpi.pin[%d]=%s\n", pin, it->second.c_str());
			// test ob parsbar:
			parseExpr->getResult(it->second, 0, 0, 0);
			pinMode(pin, OUTPUT);
			this->pins[pin]=it->second;
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

	pinMode(PIN_PWM, PWM_OUTPUT);
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
		if(this->nFunc >= 9 && this->currentFunc[9]) { // motor FullSpeed ignorieren
			pwm = f_speed*((double)255 - this->motorStart)/255 + this->motorStart;
		} else {
			pwm = f_speed*((double)this->motorFullSpeed - this->motorStart)/255 + this->motorStart;
		}
	}
	// int result = 0;
	if(this->pwm!=pwm) {
		printf("setting pwm: %d\n", pwm);
		pwmWrite(PIN_PWM, pwm);
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
		bool value=parseExpr->getResult(it->second, this->dir, this->pwm, F0);
		// printf("digitalWrite[%d] => %d (%s)\n",it->first,value,it->second.c_str());
		digitalWrite(it->first, value);
	}
}
