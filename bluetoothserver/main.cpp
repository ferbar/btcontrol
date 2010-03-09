/*
 *  This file is part of btcontroll
 *  btcontroll is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  btcontroll is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with btcontroll.  If not, see <http://www.gnu.org/licenses/>.
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

#include <pthread.h>

#include <map>

#include "../velleman_usb_k8055/k8055.h"

#include "srcp.h"

#include "server.h"
#include "lokdef.h"
#include "message_layout.h"



// macht aus blah "blah"
#define     _STR(x)   _VAL(x)
#define     _VAL(x)   #x


// mit -D `svnversion` übergeben
#ifndef SVNVERSION
#define SVNVERSION unknown
#endif

bool cfg_debug=false;
bool cfg_dump=false;
const char *cfg_hostname="127.0.0.1";
int cfg_port=4303;

int protocolHash;

K8055 *platine=NULL;
SRCP *srcp=NULL;

std::string readFile(std::string filename)
{
	std::string ret;
	struct stat buf;
	if(stat(filename.c_str(), &buf) == 0) {
		ret.resize(buf.st_size,'\0');
		FILE *f=fopen(filename.c_str(),"r");
		if(!f) {
			fprintf(stderr,"error reading file %s\n",filename.c_str());
		} else {
			const char *data=ret.data(); // mutig ...
			fread((void*)data,1,buf.st_size,f);
			fclose(f);
			printf("%s:%lu bytes\n",filename.c_str(),buf.st_size);
		}
	} else {
		fprintf(stderr,"error stat file %s\n",filename.c_str());
	}
	return ret;
}

void resetPlatine()
{
	if(platine)
		platine->write_output(255, 255, 0xaa);
}

void signalHandler(int signo, siginfo_t *p, void *ucontext)
{
	printf("signalHandler\n");
	// TODO: programm nicht gleich killen - exit-msg an die clients schicken
	resetPlatine();
	if(srcp) {
		delete(srcp);
		srcp=NULL;
	}
	exit(0);
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
			{0, 0, 0, 0}
		};
		c = getopt_long(argc, argv, "hduvs:p:", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case 'u':
				cfg_dump=1;
				/* no break */	
			case 'd':
				cfg_debug=1;
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
			default:
				case 'h':
				printf("btserver\n"
					"	--debug\n"
					"	--version\n"
					"	--help\n"
					"   --dump\tdump protocol.dat, lokdef.csv\n"
					"\n"
					"connected zur conrad usb platine (PWM) oder erdcc (DCC)\n");
				exit(0);
		}
	}
	// FBTCtlMessage test(FBTCtlMessage::STRUCT)
	try {
		protocolHash = loadMessageLayouts();
		printf("---------------protohash = %d\n",protocolHash);
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
	} catch(std::string &s) {
		printf("exception %s\n",s.c_str());
	}

	if( ! readLokdef() ) {
		exit(1);
	}
	if(cfg_dump) {
		dumpLokdef();
		dumpMessageLayouts();
	}

	struct sigaction sa;
	memset(&sa,0,sizeof(sa));
	sa.sa_sigaction=signalHandler;
	sa.sa_flags=SA_SIGINFO;
	sigaction(SIGINT,&sa,NULL);
	sigaction(SIGTERM,&sa,NULL);
	printf("btserver starting...\n");

	try {
		assert(!platine);
		platine=new K8055(1,cfg_debug);
		printf("init platine\n");
		resetPlatine();
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


	Server server;
	server.run();

	if(platine) {
		resetPlatine();
		delete platine;
	}

}
