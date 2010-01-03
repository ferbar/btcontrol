/**
 * hint: http://www.mulliner.org/bluetooth/btchat/
 * 
 */
#include <stdio.h>
#include <stdlib.h>
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

#include "../usb k8055/k8055.h"

#include "srcp.h"

#include "server.h"
#include "clientthread.h"
#include "lokdef.h"



// macht aus blah "blah"
#define     _STR(x)   _VAL(x)
#define     _VAL(x)   #x


// mit -D `svnversion` übergeben
#ifndef SVNVERSION
#define SVNVERSION unknown
#endif

bool cfg_debug=false;

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
			printf("%s:%d bytes\n",filename.c_str(),buf.st_size);
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


/**
 * für jedes handy ein eigener thread...
 */
void *phoneClient(void *data)
{
	startupdata_t *startupdata=(startupdata_t *)data;
	printf("%d:new client\n",startupdata->clientID,startupdata->clientID);

	try {
		ClientThread client(startupdata->clientID, startupdata->so);
		client.run();
	} catch(const char *e) {
		printf("%d:exception %s\n",startupdata->clientID,e);
	} catch(std::string &s) {
		printf("%d:exception %s\n",startupdata->clientID,s.c_str());
	}
	printf("%d:client exit\n",startupdata->clientID);
	close(startupdata->so);
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

	if(argc > 1) {
		if(strcmp(argv[1],"--debug")==0) {
			cfg_debug=1;
		}
		if(strcmp(argv[1],"--version")==0) {
			printf("btserver version %s\n", _STR(SVNVERSION));
			exit(0);
		}
		if(strcmp(argv[1],"--help")==0) {
			printf("btserver\n"
				"	--debug\n"
				"	--version\n"
				"	--help\n"
				"\n"
				"connected zur conrad usb platine (PWM) oder erdcc (DCC)\n");
			exit(0);
		}
	}

	if( ! readLokdef() ) {
		exit(1);
	}
	dumpLokdef();

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
	while(1) {
		int nsk = server.accept();
	// client thread vorbereiten + starten
		startupdata_t *startupdata=(startupdata_t*) calloc(sizeof(startupdata_t),1);
		startupdata->clientID=server.clientID_counter++;
		startupdata->so=nsk;
		pthread_t &newThread=server.clients[startupdata->clientID];
		bzero(&newThread,sizeof(newThread));
		pthread_create(&newThread, NULL, phoneClient, (void *)startupdata);
	}

	if(platine) {
		resetPlatine();
		delete platine;
	}

}
