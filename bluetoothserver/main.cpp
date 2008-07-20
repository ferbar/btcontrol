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
	const char *name;
	bool pulse; // nur einmal einschalten dann gleich wieder aus
	bool ison;
};


struct lokdef_t {
	int addr;
	char name[12];
	int nFunc;
	func_t func[16];
	int currspeed;
};

static lokdef_t lokdef[16] = {
//	{1,"lok1"},
//	{2,"lok2"},
//	{3,"lok3",4,{{"func1"},{"func2"},{"func3"},{"func4"}}},
// die funk 5-12 werden extra übertragen -> sollten nicht stören
	{3,"lok3",12,{{"func1"},{"func2"},{"func3"},{"func4"},{"func5"},{"func6"},{"func7"},{"func8"},{"func9"},{"func10"},{"func11"},{"func12"}}},
	{4,"Ge 4/4 I",12,{{"sPfeife",true},{"sBremse",true},{"sPfeife+Echo"},{"sAnsage"},{"sAggregat ein/aus",false},{"sSound ein/aus",false},{"lFührerstand"},{"Rangiergang"},{"Ansage",true},
		{"sLufthahn"},{"Pantho"},{"Pantho"},{"sKompressor"},{"sVakuumpumpe"},{"sStufenschalter"},{"schaltbare Verzögerung"}}},
	{5,"lok5"},
	{6,"Ge 6/6 II",8,{{"sSound ein/aus"},{"sPfeife"},{"sRangierpfiff"},{"sKompressor"},{"sPressluft"},{"Rangiergang"}}},
	{413,"Ge 6/6 I",12,{{"lFührerstand"},{"2"},{"3"},{"lFührerstand"},{"sPfeife",true},{"s6"},{"s7"},{"sSound ein/aus"},{"sPfeife"},{"sRangierpfiff"},{"sKompressor"},{"sPressluft"}}},
	{7,"Ge 4/4 II",8,{{"lFührerstand"},{"lFührerstand"},{"lSchweizer Rücklicht"},{"???"}}},
	{12,"Ge 2/4"},
	{14,"schoema",4,{{""},{"lblink"},{"lblink"},{""}}},
	{0}
};

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
	int speed=0;
	int addr=3;
	while(1) {
		char buffer[256];
		int size;
		bool emergencyStop=false;

		if((size = read(startupdata->so, buffer, sizeof(buffer))) <= 0) {
			printf("%d:error reading message\n",startupdata->clientID);
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
				speed+=5;
				if(speed > 255)
					speed=255;
			} else if(memcmp(cmd,"down",4)==0) {
				speed-=5;
				if(speed < -255)
					speed=-255;
			} else if(STREQ(cmd,"list")) { // liste mit eingetragenen loks abrufen, format: <name>;<adresse>;...\n
				int i=0;
				char buffer[32];
				while(lokdef[i].addr) {
					snprintf(buffer,sizeof(buffer)," %d;%s\n",lokdef[i].addr,lokdef[i].name);
					sendToPhone(buffer);
					i++;
				}
				/*
				sendToPhone(" lok2;2\n");
				sendToPhone(" lok3;3\n");
				sendToPhone(" Ge 4/4 I;4\n");
				sendToPhone(" lok5;5\n");
				sendToPhone(" Ge 4/4 II;7\n");
				sendToPhone(" Ge 6/6 II;6\n");
				sendToPhone(" Ge 2/4;12\n");
				sendToPhone(" schoema;14\n");
				*/
			} else if(STREQ(cmd,"funclist")) {
				int i=0;
				printf("funclist for addr %d\n");
				while(lokdef[i].addr) {
					if(lokdef[i].addr==addr) {
						char buffer[32];
						for(int j=0; j < lokdef[i].nFunc; j++) {
							snprintf(buffer,sizeof(buffer)," %d;%d;%s\n",j+1,lokdef[i].func[j].ison,lokdef[i].func[j].name);
							sendToPhone(buffer);
						}
						break;
					}
					i++;
				}
			} else if(STREQ(cmd,"func")) { // wenn cmd kürzer als 5 zeichen bricht der ja beim \0 ab -> ok
				char *endptr=NULL;
				int funcNr=atol(param1); // geht von 1..16
				int i=0;
				while(lokdef[i].addr) {
					if(lokdef[i].addr==addr) {
						if(funcNr > 0 && funcNr <= lokdef[i].nFunc) {
							if(STREQ(param2,"on"))
								lokdef[i].func[funcNr-1].ison = true;
							else
								lokdef[i].func[funcNr-1].ison = false;
						} else {
							printf("%d:invalid funcNr out of bounds(%d)\n",startupdata->clientID,funcNr);
						}
						break;
					}
					i++;
				}
			} else if(STREQ(cmd,"stop")) {
				printf("stop\n");
				speed=0;
			} else if(STREQ(cmd,"select")) { // ret = lokname
				addr=atoi(param1);
				printf("%d:neue lok addr:%d\n",startupdata->clientID,addr);
				int i=0;
				bool found=false;
				while(lokdef[i].addr) {
					if(lokdef[i].addr==addr) {
						char buffer[32];
						snprintf(buffer,sizeof(buffer)," %s\n",lokdef[i].name);
						sendToPhone(buffer);
						found=true;
						break;
					}
					i++;
				}
				if(!found)
					sendToPhone(" invalid id\n");
			} else if(STREQ(cmd,"pwr_off")) {
				if(srcp) srcp->pwrOff();
			} else if(STREQ(cmd,"pwr_on")) {
				if(srcp) srcp->pwrOn();
			} else if(memcmp(cmd,"invalid_key",10)==0) {
				printf("%d:invalid key ! param1: %s\n",startupdata->clientID,param1);
				int i=0;
				int row=-1;
				while(lokdef[i].addr) {
					if(lokdef[i].addr==addr) {
						row=i;
						break;
					}
					i++;
				}
				bool notaus=false;
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
				if(notaus) {
					printf("notaus\n");
					speed=0;
					emergencyStop=true;
				}
			}
		} else {
			printf("%d:no command?????",startupdata->clientID);
		}

		// REPLY SENDEN -----------------
		snprintf(buffer,sizeof(buffer),"%d %d\n",nr,speed);
		sendToPhone(buffer);

		// PLATINE ANSTEUERN ------------
		if(platine) {
			// geschwindigkeit 
			// double f_speed=sqrt(sqrt((double)speed/255.0))*255.0; // für üperhaupt keine elektronik vorm motor gut (schienentraktor)
			unsigned int a_speed=abs(speed);
			double f_speed=a_speed;

			int ia1=255-(int)f_speed;
			printf("%d:speed: %d (%f)\n",startupdata->clientID,ia1,f_speed);
			int ia2=speed < 0 ? 255 : 0; // 255 -> relais zieht an
			int id8=0;
			// printf("speed=%d: ",speed);
			for(int i=1; i < 9; i++) {
				// printf("%d ",255*i/(9));
				if( a_speed >= 255*i/(9)) {
					id8 |= 1 << (i-1);
				}
			}
			// printf("\n");
			if(speed==0) { // lauflicht anzeigen
				int a=1 << (nr&3);
				id8=a | a << 4;
			}
			platine->write_output(ia1, ia2, id8);
		} else if(srcp) { // erddcd/srcp/dcc:
			int dir= speed < 0 ? 0 : 1;
			if(emergencyStop) {
				dir=2;
			}
			int i=0;
			bool found=false;
			while(lokdef[i].addr) {
				if(lokdef[i].addr==addr) {
					// int nFahrstufen = 14;
					int nFahrstufen = 127;
					int dccSpeed = abs(speed) * nFahrstufen / 255;
					bool func[16];
					for(int j=0; j < lokdef[i].nFunc; j++) {
						func[j]=lokdef[i].func[j].ison;
					}
					srcp->send(addr, dir, nFahrstufen, dccSpeed, lokdef[i].nFunc, func);
					found=true;
					break;
				}
				i++;
			}
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
	struct sigaction sa;
	memset(&sa,0,sizeof(sa));
	sa.sa_sigaction=signalHandler;
	sa.sa_flags=SA_SIGINFO;
	sigaction(SIGINT,&sa,NULL);
	sigaction(SIGTERM,&sa,NULL);
	printf("dudl\n");
	bdaddr_t bdaddr;
	bacpy(&bdaddr, BDADDR_ANY);

	int ctl=1000;
	cmd_listen(ctl, 30, &bdaddr, 30);
	if(platine)
		delete platine;

}
