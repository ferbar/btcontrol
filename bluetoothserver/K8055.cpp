/*
 *  This file is part of btcontrol
 *
 *  Copyright (C) Christian Ferbar
 *
 *  btcontrol is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  btcontrol is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with btcontrol.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Vellemann K8055 interface
 */
#include "K8055.h"
#include "../jodersky_k8055/src/k8055.h"
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdexcept>
#include <string>
#include "utils.h"

/**
 *
 * wenn das dauernd errors meldet a) kabel checken
 * b) timeout und retry raufdrehn
 *
 */


K8055::K8055(int devnr, bool debug) :
	USBPlatine(debug) {
	k8055_debug(true);
	// devnr war beim alten 1...4 da isses jetzt 0...3
	int rc=k8055_open_device(devnr-1, &this->dev);
	if(rc != K8055_SUCCESS) {
		printf("----rc=%d\n",rc);
		throw std::runtime_error("error open velleman device (" + utils::to_string(rc) + ")");
	}
}

K8055::~K8055() {
	k8055_close_device(this->dev);
	this->dev=NULL;
}

void K8055::setPWM(int f_speed) {
	// Umrechnen in PWM einheiten
	// Velleman: K8055 
	const double motorStart=180;
	const double fullSpeed=128; // DCC full speed
	// 255 = pwm max
	unsigned char pwm = 0;
	if(f_speed > 0) {
		pwm = f_speed*(fullSpeed-motorStart)/255+motorStart;
	}
	this->pwm = pwm;
}

void K8055::setDir(unsigned char dir) {
	this->dir=dir;
}

void K8055::commit() {

	int ia1=255-(int)this->pwm;
	int ia2=0;
	int id8=0;
	if( this->dir < 0 ) { // vorsicht: höchste adresse blinkt beim booten!
		id8=0x40;
	} else {
		id8=0x20;
	}
	// printf("lokdef[addr_index].currspeed=%d: ",lokdef[addr_index].currspeed);
	/* geschwindigkeits led balken anzeigen:
	for(int i=1; i < 9; i++) {
		// printf("%d ",255*i/(9));
		if( a_speed >= 255*i/(9)) {
			id8 |= 1 << (i-1);
		}
	} */
	// printf("\n");
	/*
	// lauflicht anzeigen
	if(lokdef[addr_index].currspeed==0) {
		int a=1 << (nr&3);
		// id8=a | a << 4;
		id8 |= a << 4;
	}
	*/

	this->write_output(ia1, ia2, id8);
	unsigned char a1, a2, d; short unsigned int c1, c2;
	if(this->read_input(&a1, &a2, &d, &c1, &c2 ) ) // irgendwas einlesen - write_output und dann gleich ein close => kommt nie an
		printf("read: a1: %u, a2:%u, d:%x, counter1:%u, counter2:%u\n", a1, a2, d, c1, c2);

	this->write_output(ia1, ia2, id8);
}

void K8055::fullstop() {
	this->write_output(0, 0, 3);
}

void K8055::benchmark() {
	assert(this->dev);
	printf("=========================K8055::benchmark()\n");
#define ITERATIONS 100
#define device this->dev
	struct timeval t0;
	struct timeval t;

	int us = 0;
	int err=0;
	for (int i = 0; i < ITERATIONS; ++i) {
		if(k8055_set_all_digital(device,0) || k8055_set_all_analog(device, 255,0)) {
			printf("eout\n");
		}
		gettimeofday(&t0, NULL);
		if(k8055_get_all_input(device, NULL, NULL, NULL, NULL, NULL, false) != 0) {
			err++;
			printf("err\n");
		} else {
			printf(".");fflush(stdout);
		}
		gettimeofday(&t, NULL);
		us += (t.tv_sec - t0.tv_sec) * 1000000 + t.tv_usec - t0.tv_usec;
		// sleep(1);
	}
	printf("average read time for %i iterations: %.3f [ms] err=%d\n", ITERATIONS, 1.0 * us / ITERATIONS / 1000, err);

}
void K8055::onebench() {
	assert(this->dev);
	struct timeval t0;
	struct timeval t;

	int us = 0;
	int err=0;
		if(k8055_set_all_digital(device,0) || k8055_set_all_analog(device, 255,0)) {
			printf("eout\n");
		}
		gettimeofday(&t0, NULL);
		if(k8055_get_all_input(device, NULL, NULL, NULL, NULL, NULL, false) != 0) {
			printf("err\n");
			err++;
		} else {
			printf(".");fflush(stdout);
		}
		gettimeofday(&t, NULL);
		us += (t.tv_sec - t0.tv_sec) * 1000000 + t.tv_usec - t0.tv_usec;
}

int K8055::takeover_device( int interface ) {
	assert(!"implemented");
}

int K8055::write_empty_command ( unsigned char command ) {
	assert(!"implemented");
}

int K8055::write_dbt_command ( unsigned char command,unsigned char t1, unsigned char t2 ) {
	assert(!"implemented");
}

int K8055::write_output ( unsigned char a1, unsigned char a2, unsigned char d ) {
	assert(this->dev);
	/*
	if(k8055_set_all_digital(this->dev,0) || k8055_set_all_analog(this->dev, 255,0)) {
		printf("eout\n");
	}
	if(k8055_get_all_input(this->dev, NULL, NULL, NULL, NULL, NULL, false) != 0) {
		printf("err\n");
	} else {
		printf(".");fflush(stdout);
	}
*/
	
	int rc;
//	if(this->d != d) {
		rc=k8055_set_all_digital(this->dev,d);
		if(rc != K8055_SUCCESS) {
			printf("Error write_output1 (rc=%d,d=%0x)\n",rc,d);
		} else {
			// printf("ok    write_output1 (rc=%d,d=%0x)\n",rc,d);
		}
//	}
	rc=k8055_set_all_analog(this->dev, a1, a2);
	if(rc != K8055_SUCCESS) {
		printf("Error write_output analog (rc=%d,a1=%d,a2=%d)\n",rc,a1,a2);
		return false;
	}
	return true;
}

int K8055::read_input ( unsigned char *a1, unsigned char *a2, unsigned char *d, unsigned short *c1, unsigned short *c2 ) {
	assert(this->dev);

	int id, ia1, ia2, ic1, ic2;
	// letzter parameter == quick
	int rc=k8055_get_all_input(this->dev, &id, &ia1, &ia2, &ic1, &ic2, false);
	if(rc != K8055_SUCCESS) {
		printf("Error read_input (%d)\n",rc);
		return false;
	}
	*d=id;
	*a1=ia1;
	*a2=ia2;
	*c1=ic1;
	*c2=ic2;
	// usleep(10000);
	return true;
}

int K8055::msec_to_dbt_code ( int msec ) {
	assert(!"implemented");
}

int K8055::set_counter1_bouncetime ( int msec ) {
	assert(!"implemented");
}

int K8055::set_counter2_bouncetime ( int msec ) {
	assert(!"implemented");
}

int K8055::reset_counter1 ( ) {
	assert(!"implemented");
}

int K8055::reset_counter2 ( ) {
	assert(!"implemented");
}

