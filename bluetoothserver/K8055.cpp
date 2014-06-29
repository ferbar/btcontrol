#include "K8055.h"
#include "../jodersky_k8055/src/k8055.h"
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

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
		throw "error open dev";
	}
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

K8055::~K8055() {
	k8055_close_device(this->dev);
	this->dev=NULL;
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

