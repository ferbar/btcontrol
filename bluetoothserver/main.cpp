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

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>

#include <map>

#include "../usb k8055/k8055.h"

#include "srcp.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// macht aus blah "blah"
#define     _STR(x)   _VAL(x)
#define     _VAL(x)   #x


// mit -D `svnversion` übergeben
#ifndef SVNVERSION
#define SVNVERSION unknown
#endif

static int auth = 0;
static int encryption = 0;
static int secure = 0;
static int master = 0;
static int linger = 0;
bool cfg_debug=false;

K8055 *platine=NULL;
SRCP *srcp=NULL;

struct startupdata_t {
	int clientID;
	int so;
};

struct func_t {
	char name[32];
	bool pulse; // nur einmal einschalten dann gleich wieder aus
	bool ison;
};

#define F_DEFAULT 0
#define F_DEC14 1
#define F_DEC28 2

struct lokdef_t {
	int addr;
	int flags;		// LGB loks mit 14 fahrstufen ansteuern
	char name[12];
	int nFunc;
	func_t func[16];
	int currspeed;
	bool initDone;
};

// darf kein pointer sein weil .currspeed und func wird ja geändert
static lokdef_t *lokdef; 
/*= {
//	{1,"lok1"},
//	{2,"lok2"},
//	{3,"lok3",4,{{"func1"},{"func2"},{"func3"},{"func4"}}},
// die funk 5-12 werden extra übertragen -> sollten nicht stören
	{3,  F_DEFAULT,"lok3",12,{{"func1"},{"func2"},{"func3"},{"func4"},{"func5"},{"func6"},{"func7"},{"func8"},{"func9"},{"func10"},{"func11"},{"func12"}}},
	{4,  F_DEFAULT,"Ge 4/4 I",12,{{"sPfeife",true},{"sBremse",true},{"sPfeife+Echo"},{"sAnsage"},{"sAggregat ein/aus",false},{"sSound ein/aus",false},{"lFührerstand"},{"Rangiergang"},{"Ansage",true},
		{"sLufthahn"},{"Pantho"},{"Pantho"},{"sKompressor"},{"sVakuumpumpe"},{"sStufenschalter"},{"schaltbare Verzögerung"}}},
	{5,  F_DEFAULT,"lok5"},
	{6,  F_DEFAULT,"Ge 6/6 II",8,{{"sSound ein/aus"},{"sPfeife"},{"sRangierpfiff"},{"sKompressor"},{"sPressluft"},{"Rangiergang"}}},
	{413,F_DEFAULT,"Ge 6/6 I",12,{{"lFührerstand"},{"2"},{"3"},{"lFührerstand"},{"sPfeife",true},{"s6"},{"s7"},{"sSound ein/aus"},{"sPfeife"},{"sRangierpfiff"},{"sKompressor"},{"sPressluft"}}},
	{108,F_DEFAULT,"G 4/5",12,{{"lFührerstand"},{"2"},{"3"},{"lFührerstand"},{"sPfeife",true},{"s6"},{"s7"},{"sSound ein/aus"},{"sPfeife"},{"sRangierpfiff"},{"sKompressor"},{"sPressluft"}}},
	{7,  F_DEFAULT,"Ge 4/4 II",8,{{"lFührerstand"},{"lFührerstand"},{"lSchweizer Rücklicht"},{"???"}}},
	{647,F_DEFAULT,"Ge 4/4 III",12,{{"lFührerstand"},{"2"},{"3"},{"lFührerstand"},{"sPfeife",true},{"s6"},{"s7"},{"sSound ein/aus"},{"sPfeife"},{"sRangierpfiff"},{"sKompressor"},{"sPressluft"}}},
	{203,F_DEFAULT,"Ge 2/6 203",12,{{"lFührerstand"},{"2"},{"3"},{"lFührerstand"},{"sPfeife",true},{"s6"},{"s7"},{"sSound ein/aus"},{"sPfeife"},{"sRangierpfiff"},{"sKompressor"},{"sPressluft"}}},
	{12, F_LGB_DEC,"Ge 2/4 213"},
	{14, F_DEFAULT,"schoema",4,{{""},{"lblink"},{"lblink"},{""}}},
	{0}
};
*/

/**
 * liefert den index
 * @return < 0 wenn lok nicht gefunden
 *
 */
int getAddrIndex(int addr)
{
	int i=0;
	while(lokdef[i].addr) {
		if(lokdef[i].addr==addr) {
			return i;
		}
		i++;
	}
	return -1;
}

void resetPlatine()
{
	if(platine)
		platine->write_output(255, 255, 0xaa);
}

#define STREQ(s1,s2) (strcmp(s1,s2)==0)

#define sendToPhone(text) \
		if(strlen(text) != write(startupdata->so,text,strlen(text))) { \
			printf("%d:error writing message (%s)\n",startupdata->clientID, strerror(errno)); \
			break; \
		} else { \
			printf("%d: ->%s",startupdata->clientID,text); \
		}

#define MAXPATHLEN 255

/**
 * für jedes handy ein eigener thread...
 */
void *phoneClient(void *data)
{
	startupdata_t *startupdata=(startupdata_t *)data;
	printf("%d:new client ID=%d\n",startupdata->clientID,startupdata->clientID);
	printf("%d:socket accepted sending welcome msg\n",startupdata->clientID);
	write(startupdata->so,"hallo\n",6);


	int x=0;
	// int speed=0;
	int addr=3;
	while(1) {
		int addr_index=getAddrIndex(addr);
		// addr_index sollte immer gültig sin - sonst gibts an segv ....
		
		char buffer[256];
		int size;
		bool emergencyStop=false;

		if((size = read(startupdata->so, buffer, sizeof(buffer))) <= 0) {
			printf("%d:error reading message size=%d \"%.*s\"\n",startupdata->clientID,size,size >= 0 ? buffer : NULL);
			break;
		}
		buffer[size]='\0';
		printf("%.*s",size,buffer);
		int nr=0;
		char cmd[sizeof(buffer)]="";
		char param1[sizeof(buffer)]="";
		char param2[sizeof(buffer)]="";
		sscanf(buffer,"%d %s %s %s",&nr,&cmd, &param1, &param2);
		printf("%d:<- %s p1=%s p2=%s\n",startupdata->clientID,cmd,param1,param2);
		
		if(cmd) {
			if(memcmp(cmd,"up",2)==0) {
				lokdef[addr_index].currspeed+=5;
				if(lokdef[addr_index].currspeed > 255)
					lokdef[addr_index].currspeed=255;
			} else if(memcmp(cmd,"down",4)==0) {
				lokdef[addr_index].currspeed-=5;
				if(lokdef[addr_index].currspeed < -255)
					lokdef[addr_index].currspeed=-255;
			} else if(STREQ(cmd,"list")) { // liste mit eingetragenen loks abrufen, format: <name>;<adresse>;...\n
				int i=0;
				char buffer[32];
				while(lokdef[i].addr) {
					snprintf(buffer,sizeof(buffer)," %d;%s\n",lokdef[i].addr,lokdef[i].name);
					sendToPhone(buffer);
					i++;
				}
			} else if(STREQ(cmd,"funclist")) {
				printf("funclist for addr %d\n",addr);
				char buffer[32];
				for(int j=0; j < lokdef[addr_index].nFunc; j++) {
					snprintf(buffer,sizeof(buffer)," %d;%d;%s\n",j+1,lokdef[addr_index].func[j].ison,lokdef[addr_index].func[j].name);
					sendToPhone(buffer);
				}
			} else if(STREQ(cmd,"func")) { // wenn cmd kürzer als 5 zeichen bricht der ja beim \0 ab -> ok
				char *endptr=NULL;
				int funcNr=atol(param1); // geht von 1..16

				if(funcNr > 0 && funcNr <= lokdef[addr_index].nFunc) {
					if(STREQ(param2,"on") || STREQ(param2,"down"))
						lokdef[addr_index].func[funcNr-1].ison = true;
					else
						lokdef[addr_index].func[funcNr-1].ison = false;
				} else {
					printf("%d:invalid funcNr out of bounds(%d)\n",startupdata->clientID,funcNr);
				}
			} else if(STREQ(cmd,"stop")) {
				printf("stop\n");
				lokdef[addr_index].currspeed=0;
			} else if(STREQ(cmd,"select")) { // ret = lokname
				int new_addr=atoi(param1);
				printf("%d:neue lok addr:%d\n",startupdata->clientID,new_addr);
				int new_addr_index=getAddrIndex(new_addr);
				if(new_addr_index >= 0) {
					addr=new_addr;
					addr_index=new_addr_index;
					snprintf(buffer,sizeof(buffer)," %s\n",lokdef[addr_index].name);
					sendToPhone(buffer);
				} else {
					sendToPhone(" invalid id\n");
				}
			} else if(STREQ(cmd,"pwr_off")) {
				if(srcp) srcp->pwrOff();
			} else if(STREQ(cmd,"pwr_on")) {
				if(srcp) srcp->pwrOn();
			} else if(memcmp(cmd,"invalid_key",10)==0) {
				printf("%d:invalid key ! param1: %s\n",startupdata->clientID,param1);
				bool notaus=false;
				/*
				int i=0;
				int row=-1;
				while(lokdef[i].addr) {
					if(lokdef[i].addr==addr) {
						row=i;
						break;
					}
					i++;
				}
				if(row >= 0) {
					if(strcmp(param1,"(49)")==0) {
						lokdef[i].func[0].ison = ! lokdef[i].func[0].ison;
					} else if(strcmp(param1,"(55)")==0) {
						lokdef[i].func[1].ison = ! lokdef[i].func[1].ison;
					} else if(strcmp(param1,"(51)")==0) {
						lokdef[i].func[2].ison = ! lokdef[i].func[2].ison;
					} else if(strcmp(param1,"(57)")==0) {
						lokdef[i].func[3].ison = ! lokdef[i].func[3].ison;
					} else {
						notaus=true;
					}
				} else {
					notaus=true;
				}
				*/
				if(notaus) {
					printf("notaus\n");
					lokdef[addr_index].currspeed=0;
					emergencyStop=true;
				}
			}
		} else {
			printf("%d:no command?????",startupdata->clientID);
		}

		// REPLY SENDEN -----------------
		int funcbits=0;
		for(int i=0; i < lokdef[addr_index].nFunc; i++) {
			if(lokdef[addr_index].func[i].ison) {
				funcbits |= (1 << i);
			}
		}
		snprintf(buffer,sizeof(buffer),"%d %d %d %04x\n",nr,
			lokdef[addr_index].addr,lokdef[addr_index].currspeed,funcbits);
		sendToPhone(buffer);

		// PLATINE ANSTEUERN ------------
		if(platine) {
			// geschwindigkeit 
			// double f_speed=sqrt(sqrt((double)lokdef[addr_index].currspeed/255.0))*255.0; // für üperhaupt keine elektronik vorm motor gut (schienentraktor)
			unsigned int a_speed=abs(lokdef[addr_index].currspeed);
			double f_speed=a_speed;

			int ia1=255-(int)f_speed;
			printf("%d:lokdef[addr_index].currspeed: %d (%f)\n",startupdata->clientID,ia1,f_speed);
			int ia2=lokdef[addr_index].currspeed < 0 ? 255 : 0; // 255 -> relais zieht an
			int id8=0;
			// printf("lokdef[addr_index].currspeed=%d: ",lokdef[addr_index].currspeed);
			for(int i=1; i < 9; i++) {
				// printf("%d ",255*i/(9));
				if( a_speed >= 255*i/(9)) {
					id8 |= 1 << (i-1);
				}
			}
			// printf("\n");
			if(lokdef[addr_index].currspeed==0) { // lauflicht anzeigen
				int a=1 << (nr&3);
				id8=a | a << 4;
			}
			platine->write_output(ia1, ia2, id8);
		} else if(srcp) { // erddcd/srcp/dcc:
			int dir= lokdef[addr_index].currspeed < 0 ? 0 : 1;
			if(emergencyStop) {
				dir=2;
			}
			// int nFahrstufen = 14;
			int nFahrstufen = 127;
			if(lokdef[addr_index].flags & F_DEC14) {
				nFahrstufen = 14;
			} else if(lokdef[addr_index].flags & F_DEC28) {
				nFahrstufen = 28;
			}
			int dccSpeed = abs(lokdef[addr_index].currspeed) * nFahrstufen / 255;
			bool func[16];
			for(int j=0; j < lokdef[addr_index].nFunc; j++) {
				func[j]=lokdef[addr_index].func[j].ison;
			}
			if(!lokdef[addr_index].initDone) {
				srcp->sendLocoInit(addr, nFahrstufen, lokdef[addr_index].nFunc);
				lokdef[addr_index].initDone=true;
			}
			srcp->sendLocoSpeed(addr, dir, nFahrstufen, dccSpeed, lokdef[addr_index].nFunc, func);
		}
	}
	printf("%d:client exit\n",startupdata->clientID);
}

/**
 *
 *
 */
// mehr oder weniger aus http://bluez.cvs.sourceforge.net/*checkout*/bluez/utils/rfcomm/main.c?revision=1.31
static void cmd_listen(int ctl, int dev, bdaddr_t *bdaddr, int rc_channel)
{
	struct sockaddr_rc laddr, raddr;
	struct rfcomm_dev_req req;
//	struct termios ti;
//	struct sigaction sa;
//	struct pollfd p;
	sigset_t sigs;
	socklen_t alen;
	char dst[18], devname[MAXPATHLEN]="";
	int sk, nsk, lm, ntry = 30;

	laddr.rc_family = AF_BLUETOOTH;
	bacpy(&laddr.rc_bdaddr, bdaddr);
	laddr.rc_channel = rc_channel;

	sk = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if (sk < 0) {
		perror("Can't create RFCOMM socket");
		return;
	}

	lm = 0;
	if (master)
		lm |= RFCOMM_LM_MASTER;
	if (auth)
		lm |= RFCOMM_LM_AUTH;
	if (encryption)
		lm |= RFCOMM_LM_ENCRYPT;
	if (secure)
		lm |= RFCOMM_LM_SECURE;

	if (lm && setsockopt(sk, SOL_RFCOMM, RFCOMM_LM, &lm, sizeof(lm)) < 0) {
		perror("Can't set RFCOMM link mode");
		close(sk);
		return;
	}

	if (bind(sk, (struct sockaddr *)&laddr, sizeof(laddr)) < 0) {
		perror("Can't bind RFCOMM socket");
		close(sk);
		return;
	}

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

	int clientID_counter=1;
	std::map<int,pthread_t> clients;

	listen(sk, 10);
while(1) {
	printf("Waiting for connection on channel %d\n", laddr.rc_channel);

	alen = sizeof(raddr);
	nsk = accept(sk, (struct sockaddr *) &raddr, &alen);

// brauch ma das noch für irgendwas????
	alen = sizeof(laddr);
	if (getsockname(nsk, (struct sockaddr *)&laddr, &alen) < 0) {
		perror("Can't get RFCOMM socket name");
		close(nsk);
		return;
	}

	memset(&req, 0, sizeof(req));
	bacpy(&req.src, &laddr.rc_bdaddr);
	bacpy(&req.dst, &raddr.rc_bdaddr);
	req.channel = raddr.rc_channel;
	ba2str(&req.dst, dst);
	printf("Connection from %s to %s\n", dst, devname);

// client thread vorbereiten + starten
	startupdata_t *startupdata=(startupdata_t*) calloc(sizeof(startupdata_t),1);
	startupdata->clientID=clientID_counter++;
	startupdata->so=nsk;
	pthread_t &newThread=clients[startupdata->clientID];
	bzero(&newThread,sizeof(newThread));
	pthread_create(&newThread, NULL, phoneClient, (void *)startupdata);

	}

	resetPlatine();

	printf("Disconnected\n");
	close(nsk);

#if 0 
	if (linger) {
		struct linger l = { l_onoff = 1, l_linger = linger };

		if (setsockopt(nsk, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) < 0) {
			perror("Can't set linger option");
			close(nsk);
			return;
		}
	}
#endif

// serial tty ding anlegen ??????????????????
/*
	memset(&req, 0, sizeof(req));
	req.dev_id = dev;
	req.flags = (1 << RFCOMM_REUSE_DLC) | (1 << RFCOMM_RELEASE_ONHUP);

	bacpy(&req.src, &laddr.rc_bdaddr);
	bacpy(&req.dst, &raddr.rc_bdaddr);
	req.channel = raddr.rc_channel;

	dev = ioctl(nsk, RFCOMMCREATEDEV, &req);
	if (dev < 0) {
		perror("Can't create RFCOMM TTY");
		close(sk);
		return;
	}

	snprintf(devname, MAXPATHLEN - 1, "/dev/rfcomm%d", dev);
	while ((fd = open(devname, O_RDONLY | O_NOCTTY)) < 0) {
		if (errno == EACCES) {
			perror("Can't open RFCOMM device");
			goto release;
		}

		snprintf(devname, MAXPATHLEN - 1, "/dev/bluetooth/rfcomm/%d", dev);
		if ((fd = open(devname, O_RDONLY | O_NOCTTY)) < 0) {
			if (ntry--) {
				snprintf(devname, MAXPATHLEN - 1, "/dev/rfcomm%d", dev);
				usleep(100);
				continue;
			}
			perror("Can't open RFCOMM device");
			goto release;
		}
	}

*/

#if 0
	if (rfcomm_raw_tty) {
		tcflush(fd, TCIOFLUSH);

		cfmakeraw(&ti);
		tcsetattr(fd, TCSANOW, &ti);
	}
#endif

	close(sk);


/*
	memset(&sa, 0, sizeof(sa));
	sa.sa_flags   = SA_NOCLDSTOP;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGCHLD, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);

	sa.sa_handler = sig_term;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT,  &sa, NULL);

	sa.sa_handler = sig_hup;
	sigaction(SIGHUP, &sa, NULL);

	sigfillset(&sigs);
	sigdelset(&sigs, SIGCHLD);
	sigdelset(&sigs, SIGPIPE);
	sigdelset(&sigs, SIGTERM);
	sigdelset(&sigs, SIGINT);
	sigdelset(&sigs, SIGHUP); */

/*
	p.fd = fd;
	p.events = POLLERR | POLLHUP;

	if (argc <= 2) {
		while (!__io_canceled) {
			p.revents = 0;
			if (ppoll(&p, 1, NULL, &sigs) > 0)
				break;
		}
	} else
		run_cmdline(&p, &sigs, devname, argc - 2, argv + 2);
*/

/*
	sa.sa_handler = NULL;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT,  &sa, NULL);
	close(fd);
	*/


	return;

release:
	memset(&req, 0, sizeof(req));
	req.dev_id = dev;
	req.flags = (1 << RFCOMM_HANGUP_NOW);
	ioctl(ctl, RFCOMMRELEASEDEV, &req);

	close(sk);
}

void signalHandler(int signo, siginfo_t *p, void *ucontext)
{
	printf("signalHandler\n"); 
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
	printf("skipping value:");
	while(**pos && (strchr(",\r\n",**pos) == NULL)) {
		printf("%c",**pos);
		(*pos)++;
	}
	printf("\n");
	if(!**pos) {*pos=NULL; return NULL; }
	(*pos)++; // , überspringen
	printf("skipping spaces:");
	while(**pos && (**pos == ' ') || (**pos == '\t')) {
		printf("%c",**pos);
		(*pos)++;
	}
	printf("\n");
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
		printf("line %d\n",line);
		if(buffer[0] =='#') continue;
		lokdef = (lokdef_t *) realloc(lokdef, sizeof(lokdef_t)*(n+1));
		bzero(&lokdef[n],sizeof(lokdef_t));
		char *pos=buffer;
		char *pos_end;
		lokdef[n].addr=atoi(pos);
		pos_end=getnext(&pos);
		lokdef[n].flags=str2decodertype(pos);
		pos_end=getnext(&pos);
		strncpy(lokdef[n].name, pos,  MIN(sizeof(lokdef[n].name), pos_end-pos));
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
		printf("addr:%d flags:%d, name:%s, nFunc:%d,", lokdef[n].addr, lokdef[n].flags, lokdef[n].name, lokdef[n].nFunc);
		for(int i=0; i < lokdef[n].nFunc; i++) {
			printf("%s:%d ",lokdef[n].func[i].name,lokdef[n].func[i].ison);
		}
		printf("currspeed:%d\n",lokdef[n].currspeed);
		n++;

	}
}

int main(int argc, char *argv[])
{
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
	bdaddr_t bdaddr;
	// bzero(&bdaddr, sizeof(bdaddr));
	bacpy(&bdaddr, BDADDR_ANY);

	int ctl=1000;
	cmd_listen(ctl, 30, &bdaddr, 30);
	if(platine)
		delete platine;

}
