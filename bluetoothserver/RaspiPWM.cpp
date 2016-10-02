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

#define PIN_PWM		1

RaspiPWM::RaspiPWM(bool debug) :
	USBPlatine(debug),
	 dir(-1), pwm(-1), motorStart(70),
		fRaspiLed(NULL), raspiLedToggle(0){

	for(int i=0; i < maxPins; i++) {
		this->pinsDir1[i]=-1;
		this->pinsDir2[i]=-1;
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
}

void RaspiPWM::release() {
}

void RaspiPWM::init() {

	printf("RaspiPWM::init()\n");
	wiringPiSetup();


	std::string tmp = config.get("digispark.motorStart");
	this->motorStart=utils::stoi(tmp);
	printf("RaspiPWM::init() ---- motorStart %d\n", this->motorStart);

	int n1=0;
	int n2=0;
	for (auto it=config.begin(); it!=config.end(); ++it) {
		if(it->first == "wiringpi.dir1.pin") {
			printf("setting wiringpi.dir1.pin[%d]=%s\n", n1, it->second.c_str());
			this->pinsDir1[n1++]=utils::stoi(it->second);
		}
		if(it->first == "wiringpi.dir2.pin") {
			printf("setting wiringpi.dir2.pin[%d]=%s\n", n2, it->second.c_str());
			this->pinsDir2[n2++]=utils::stoi(it->second);
		}
		if(n1 >= this->maxPins || n2 >= this->maxPins){
			throw std::runtime_error("error reading maxPins");
		}
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
	for(int i=0; i<this->maxPins ; i++) {
		if(this->pinsDir1[i] >= 0) {
			pinMode(this->pinsDir1[i], OUTPUT);
			printf("setting OUT for pin %d\n", this->pinsDir1[i]);
		}
		if(this->pinsDir2[i] >= 0) {
			pinMode(this->pinsDir2[i], OUTPUT);
			printf("setting OUT for pin %d\n", this->pinsDir2[i]);
		}
	}
	this->setDir(1);
}

void RaspiPWM::setPWM(int f_speed) {
	// Umrechnen in PWM einheiten
	const double fullSpeed=255; // 256 is counter reset
	// 255 = pwm max
	unsigned char pwm = 0;
	if(f_speed > 0) {
		pwm = f_speed*(fullSpeed - this->motorStart)/255 + this->motorStart;
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
		for(int i=0; i<this->maxPins ; i++) {
			if(this->pinsDir1[i] >= 0)
				digitalWrite(this->pinsDir1[i], dir==1 ? 0 : 1);
			if(this->pinsDir2[i] >= 0)
				digitalWrite(this->pinsDir2[i], dir==1 ? 1 : 0);
		}
		this->dir=dir;
	}
}

void RaspiPWM::fullstop() {
	this->setPWM(0);
	printf("RaspiPWM::fullstop() done\n");
}

