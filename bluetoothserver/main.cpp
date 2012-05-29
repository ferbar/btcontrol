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

// dirname
#include <libgen.h>

#include <map>

#ifdef HAVE_LIBUSB
	#include "K8055.h"
	#include "USBDigispark.h"

	#ifdef HAVE_ALSA
		#include "zsp.h"
		#include "sound.h"
	#endif
#endif

#include "srcp.h"

#include "server.h"
#include "lokdef.h"
#include "message_layout.h"

#include "utils.h"


#include "qrcode.h"


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
#ifdef HAVE_LIBUSB
USBPlatine *platine=NULL;
#else
void *platine=NULL;
#endif
SRCP *srcp=NULL;

#undef read
int myRead(int so, void *data, size_t size) {
	int read=0;
	// printf("myRead: %zd\n",size);
	while(read < (int) size) {
		// printf("read: %zd\n",size-read);
		int rc=::read(so,((char *) data)+read,size-read);
		// printf("rc: %d\n",rc);
		if(rc < 0) {
			throw std::runtime_error("error reading data");
		} else if(rc == 0) { // stream is blocking -> sollt nie vorkommen
			throw std::runtime_error("nothing to read");
		}
		read+=rc;
	}
	return read;
}

// FIXME: das ins utils.cpp ?
std::string readFile(std::string filename)
{
	std::string ret;
	struct stat buf;
	if(stat(filename.c_str(), &buf) != 0) {
		char execpath[MAXPATHLEN];
		if(readlink("/proc/self/exe", execpath, sizeof(execpath)) <= 0) {
			printf("error reading /proc/self/exe\n");
			abort();
		}
		char *linkpath=dirname(execpath);
		filename.insert(0,std::string(linkpath) + '/');
		if(stat(filename.c_str(), &buf) != 0) {
			fprintf(stderr,"error stat file %s\n",filename.c_str());
			throw std::runtime_error("error stat file");
		}
	}
	ret.resize(buf.st_size,'\0');
	FILE *f=fopen(filename.c_str(),"r");
	if(!f) {
		fprintf(stderr,"error reading file %s\n",filename.c_str());
		throw std::runtime_error("error reading file");
	} else {
		const char *data=ret.data(); // mutig ...
		fread((void*)data,1,buf.st_size,f);
		fclose(f);
		printf("%s:%lu bytes\n",filename.c_str(),buf.st_size);
	}
	return ret;
}

/**
 * Velleman k8055 init
 */
void initPlatine()
{
#ifdef HAVE_LIBUSB
	assert(!platine);
	printf("init platine\n");
	try {
		platine=new K8055(1,cfg_debug);
	} catch(std::exception &errormsg) {
		printf("K8055: error: %s\n",errormsg.what());
	}
	try {
		platine=new USBDigispark(1,cfg_debug);
	} catch(std::exception &errormsg) {
		printf("USBDigispark init: error: %s\n",errormsg.what());
	}
	printf("... done\n");
#ifdef HAVE_ALSA
	SoundType *soundFiles=loadZSP();
	if(soundFiles) {
		Sound::loadSoundFiles(soundFiles);
	}
#endif
#endif
}

void deletePlatine()
{
#ifdef HAVE_LIBUSB
	printf("delete Platine\n");
	delete platine;
#endif
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
		messageLayouts.load();
		printf("---------------protohash = %d\n",messageLayouts.protocolHash);
		printf("TCP RX Timeout = %d\n",cfg_tcpTimeout);
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
		printf("exception %s\n",e);
		exit(1);
	} catch(std::runtime_error &e) {
		printf("exception %s\n",e.what());
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
	printf("btserver starting...\n");

#ifdef INCL_X11
	if(!cfg_X11) {
#endif
		try {
			initPlatine();
		} catch(const char *errormsg) {
			fprintf(stderr,"ansteuer platine error: %s\n",errormsg);
		}

		if(!platine) {
			assert(!srcp);
			try {
				srcp=new SRCP();
				printf("init erddcd\n");
			} catch(const char *errormsg) {
				fprintf(stderr,"error connecting to erddcd (%s)\n",errormsg);
			}
		}
#ifdef INCL_X11
	}
#endif


	Server server;
	try {
		server.run();
	} catch(std::runtime_error &e) {
		printf("exception: %s\n", e.what());
	}

	server.waitExit();

	if(platine) {
		deletePlatine();
	}

}
