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
 *
 * main file
 * hint: http://www.mulliner.org/bluetooth/btchat/
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <signal.h>
#include <assert.h>
#include <math.h>
#include <stdexcept>

#include <pthread.h>


#include <map>

#ifdef HAVE_LIBUSB 
	#include "K8055.h"
	#include "USBDigispark.h"

#endif
#ifdef HAVE_ALSA
	#include "zsp.h"
	#include "sound.h"
#endif
#if defined HAVE_RASPI_WIRINGPI || defined HAVE_RASPI_PIGPIO
	#include "RaspiPWM.h"
#endif

#include "srcp.h"

#include "server.h"
#include "lokdef.h"
#include "message_layout.h"

#include "utils.h"


#include "qrcode.h"

#define TAG "main"

// mit -D `svnversion` übergeben
#ifndef SVNVERSION
#define SVNVERSION unknown
#endif

bool cfg_debug=false;
bool cfg_dump=false;
const char *cfg_hostname="127.0.0.1";
int cfg_port=4303;

#ifdef INCL_X11
bool cfg_X11=false;
#endif

int cfg_tcpTimeout=10; // TCP RX Timeout in sekunden
Hardware *hardware=NULL;

/**
 * Velleman k8055 init
 */
void initPlatine()
{
#ifdef HAVE_LIBUSB
	assert(!hardware);
	DEBUGF("init platine");
	try {
		hardware=new K8055(1,cfg_debug);
		// FIXME:
		strncpy(lokdef[0].name,"K8055", sizeof(lokdef[0].name));
		lokdef[1].addr=0;
	} catch(std::exception &errormsg) {
		ERRORF("K8055: error: %s",errormsg.what());
	}
	try {
		hardware=new USBDigispark(1,cfg_debug);
		// Lokname immer auf RaspiPWM setzen: (unpraktisch)
		//strncpy(lokdef[0].name,"USBDigispark", sizeof(lokdef[0].name));
		lokdef[1].addr=0;
	} catch(std::exception &errormsg) {
		ERRORF("USBDigispark init: error: %s",errormsg.what());
	}
	DEBUGF("... done");
#endif
#if defined HAVE_RASPI_WIRINGPI || defined HAVE_RASPI_PIGPIO
	if(!hardware) {
		try {
			hardware=new RaspiPWM(cfg_debug);
			// Lokname immer auf RaspiPWM setzen: (unpraktisch)
			//strncpy(lokdef[0].name,"RaspiPWM", sizeof(lokdef[0].name));
			lokdef[1].addr=0;
		} catch(std::exception &errormsg) {
			ERRORF("RaspiPWM: error: %s",errormsg.what());
		}
	}
#endif
#ifdef HAVE_ALSA
	if(hardware) {
		std::string samplerate=config.get("sound.samplerate");
		if(samplerate != NOT_SET) {
			NOTICEF("new samplerate:%s",samplerate.c_str());
			Sound::sample_rate=utils::stoi(samplerate);
		}

		SoundType *soundFiles=loadZSP();
		if(soundFiles) {
			Sound::loadSoundFiles(soundFiles);
			std::string volumeLevel=config.get("sound.level");
			if(volumeLevel != NOT_SET) {
				Sound::setMasterVolume(utils::stoi(volumeLevel));
			}
		}
	}
#endif
}

void deletePlatine()
{
	NOTICEF("delete Platine");
	delete hardware;
}

void signalHandler(int signo, siginfo_t *p, void *ucontext)
{
	printf("signalHandler\n");
	/*
	// TODO: programm nicht gleich killen - exit-msg an die clients schicken
	// deletePlatine();
	if(srcp) {
		delete(srcp);
		srcp=NULL;
	}
	exit(0);
	*/
	Server::setExit();
}

int main(int argc, char *argv[])
{
	while (1) {
		int c;
		int option_index = 0;
		static struct option long_options[] = {
			{"help", 0, NULL, 'h'},
			{"debug", 0, NULL, 'd'},
			{"dump", 0, NULL, 'u'},
			{"version", 0, NULL, 'v'},
			{"server", 1, NULL, 's'},
			{"port", 1, NULL, 'p'},
#ifdef INCL_X11
			{"x11",0,NULL, 'x'},
#endif
			{0, 0, 0, 0}
		};
		c = getopt_long(argc, argv, "hduvs:p:x", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case 'u':
				cfg_dump=1;
				/* no break */	
			case 'd':
				cfg_debug=1;
				cfg_tcpTimeout=100000;
				break;
			case 'v':
				printf("btserver version %s\n", _STR(SVNVERSION));
#ifdef HAVE_RASPI_WIRINGPI
				printf("+RASPI_WIRINGPI ");
#endif
#ifdef HAVE_RASPI_PIGPIO
				printf("+RASPI_PIGPIO ");
#endif
#ifdef HAVE_ALSA
				printf("+alsa ");
#endif
#ifdef HAVE_LIBUSB
				printf("+libusb ");
#endif
#ifdef INCL_QRCODE
				printf("+INCL_QRCODE ");
#endif
#ifdef INCL_BT
				printf("+INCL_BT ");
#endif
#ifdef RASPI_ACT_LED
				printf("+RASPI_ACT_LED=" _STR(RASPI_ACT_LED) " ");
#endif
#ifdef INCL_X11
				printf("+X11 ");
#endif
				printf("\n");
				exit(0);
			case 's':
				cfg_hostname=optarg;
				break;
			case 'p':
				cfg_port=strtol(optarg, (char **) NULL, 10);
				if (errno == ERANGE || cfg_port <= 0) {
					printf("Port argument out of range: %s\n", optarg);
					exit(255);
				}
				break;
#ifdef INCL_X11
			case 'x':
				cfg_X11=true;
				break;
#endif
			default:
				case 'h':
				printf("btserver\n"
					"	[-d|--debug]\n"
					"	[-v|--version]\n"
					"	[-h--help]\n"
					"	[-u|--dump]\tdump protocol.dat, lokdef.csv\n"
					"	[-s|--server SERVER]\tSRCP server to connect to, default localhost\n"
					"	[-p|--port PORT]\tSRCP port number to connect to, default 4303\n"
#ifdef INCL_X11
					"	[-x]--x11\tX11 ctl - disables SRCP\n"
#endif
					"\n"
					"connects to Velleman k8055 USB board (PWM) or SRCP server (DCC)\n");
				exit(0);
		}
	}

#ifdef INCL_QRCODE
	printQRCode("");
#endif

	// FBTCtlMessage test(FBTCtlMessage::STRUCT)
	try {
		utils::log.init(false);

		config.init("conf/btserver.conf");
		messageLayouts.load();
		DEBUGF("---------------protohash = %d",messageLayouts.protocolHash);
		DEBUGF("TCP RX Timeout = %d",cfg_tcpTimeout);
/*
		FBTCtlMessage test(messageTypeID("PING_REPLY"));
		test["info"][0]["addr"]=1;
		test["info"][0]["speed"]=50;
		test["info"][0]["functions"]=255;
		test.dump();
		std::string msg=test.getBinaryMessage();
		printf("message: (len=%d)\n",msg.size());
		fwrite(msg.data(), 1, msg.size(), stdout);
		printf("\n------------------------\n");

		FBTCtlMessage test2(messageTypeID("GETLOCOS_REPLY"));
		test2["info"][0]["addr"]=3;
		test2["info"][0]["name"]="testding";
		test2.dump();
		msg=test2.getBinaryMessage();
		printf("message: (len=%d)\n",msg.size());
		fwrite(msg.data(), 1, msg.size(), stdout);
		printf("\n und wieder zurück ......\n");
		FBTCtlMessage test3(msg.data(), msg.size());
		test3.dump();
		exit(1);
	*/
	} catch(const char *e) {
		ERRORF("exception %s",e);
		exit(1);
	} catch(std::runtime_error &e) {
		ERRORF("exception %s",e.what());
	}

	if( ! readLokdef() ) {
		exit(1);
	}
	if(cfg_dump) {
		dumpLokdef();
		messageLayouts.dump();
	}

#ifndef NO_THREADS
	struct sigaction sa;
	memset(&sa,0,sizeof(sa));
	sa.sa_sigaction=signalHandler;
	sa.sa_flags=SA_SIGINFO;
	sigaction(SIGINT,&sa,NULL);
	sigaction(SIGTERM,&sa,NULL);
#endif
	NOTICEF("btserver starting...");

#ifdef INCL_X11
	if(!cfg_X11) {
#endif
		try {
			initPlatine();
		} catch(const char *errormsg) {
			fprintf(stderr,"ansteuer platine error: %s\n",errormsg);
		}

		if(!hardware) {
			try {
				DEBUGF("init SRCP");
				hardware=new SRCP_Hardware();
			} catch(std::runtime_error &e) {
				fprintf(stderr,"error connecting to erddcd (%s)\n", e.what());
				
				ERRORF("No hardware found ... terminating");
				abort();
			}
		}
#ifdef INCL_X11
	}
#endif

	int port=3030;
	Server server(port);
	try {
		server.run();
	} catch(std::runtime_error &e) {
		ERRORF("exception: %s", e.what());
	}

	server.waitExit();

	deletePlatine();

}
