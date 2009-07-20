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


#define MIN(a, b) ((a) < (b) ? (a) : (b))

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

/**
 * tut spaces am ende weg
 */
void strtrim(char *s)
{
	char *pos=s+strlen(s);
	while(pos > s) {
		pos--;
		if(isspace(*pos)) {
			*pos='\0';
		} else
			break;
	}
}

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


int str2decodertype(const char *pos)
{
	if(strncmp(pos,"F_DEC14",7) == 0) {
		return F_DEC14;
	}
	if(strncmp(pos,"F_DEC28",7) == 0) {
		return F_DEC28;
	}
	return F_DEFAULT;
}

char *getnext(char **pos)
{
	// printf("skipping value:");
	while(**pos && (strchr(",\r\n",**pos) == NULL)) {
		// printf("%c",**pos);
		(*pos)++;
	}
	// printf("\n");
	if(!**pos) {*pos=NULL; return NULL; }
	(*pos)++; // , überspringen
	// printf("skipping spaces:");
	while(**pos && (**pos == ' ') || (**pos == '\t')) {
		// printf("%c",**pos);
		(*pos)++;
	}
	// printf("\n");
	char *endpos=*pos;
	while(*endpos && (strchr(",\r\n",*endpos) == NULL)) endpos++;
	return endpos;
}

#define CHECKVAL(_FMT, ...)	\
		if(!pos) {	\
			fprintf(stderr,"%s:%d " _FMT " \n",LOKDEF_FILENAME,n+1, ## __VA_ARGS__);	\
			fclose(flokdef);	\
			return false;	\
		}

/**
 *
 * @return true = sussess
 */
bool readLokdef()
{
#define LOKDEF_FILENAME "lokdef.csv"
	FILE *flokdef=fopen(LOKDEF_FILENAME,"r");
	if(!flokdef) {
		fprintf(stderr,"error reading %s \n",LOKDEF_FILENAME);
		return false;
	}
	char buffer[1024];
	int n=0;
	int line=0;
	while(fgets(buffer,sizeof(buffer),flokdef)) {
		line++;
		// printf("line %d\n",line);
		if(buffer[0] =='#') continue;
		lokdef = (lokdef_t *) realloc(lokdef, sizeof(lokdef_t)*(n+1));
		bzero(&lokdef[n],sizeof(lokdef_t));
		lokdef[n].currdir=1;
		char *pos=buffer;
		char *pos_end;
		lokdef[n].addr=atoi(pos);
		pos_end=getnext(&pos);
		lokdef[n].flags=str2decodertype(pos);
		pos_end=getnext(&pos);
		strncpy(lokdef[n].name, pos,  MIN(sizeof(lokdef[n].name), pos_end-pos));
		strtrim(lokdef[n].name);
		pos_end=getnext(&pos);
		//TODO: trim
		strncpy(lokdef[n].imgname, pos, MIN(sizeof(lokdef[n].imgname), pos_end-pos));
		strtrim(lokdef[n].imgname);
		pos_end=getnext(&pos);
		CHECKVAL("error reading nfunc");
		lokdef[n].nFunc=atoi(pos);

		for(int i=0; i < lokdef[n].nFunc; i++) {
			pos_end=getnext(&pos);
			CHECKVAL("func i = %d, nfunc %d invalid? %d function names missing",i,lokdef[n].nFunc,lokdef[n].nFunc-i);
			strncpy(lokdef[n].func[i].name, pos, MIN(sizeof(lokdef[n].func[i].name), pos_end-pos));
			if(strchr(lokdef[n].func[i].name,'\n') != NULL) {
				fprintf(stderr,"%s:%d newline in funcname (%s)- irgendwas hats da\n",LOKDEF_FILENAME,n+1,lokdef[n].func[i].name);
				exit(1);
			}
		}

		n++;
	}
	fclose(flokdef);
	// listen-ende:
	lokdef = (lokdef_t *) realloc(lokdef, sizeof(lokdef_t)*(n+1));
	bzero(&lokdef[n],sizeof(lokdef_t));

	return true;
}

void dumpLokdef()
{
	int n=0;
	printf("----------------- dump lokdef -------------------------\n");
	while(lokdef[n].addr) {
		printf("addr:%d flags:%d, name:%s, img:%s, nFunc:%d,", lokdef[n].addr, lokdef[n].flags, lokdef[n].name, lokdef[n].imgname, lokdef[n].nFunc);
		for(int i=0; i < lokdef[n].nFunc; i++) {
			printf("%s:%d ",lokdef[n].func[i].name,lokdef[n].func[i].ison);
		}
		printf("currspeed:%d\n",lokdef[n].currspeed);
		n++;

	}
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
