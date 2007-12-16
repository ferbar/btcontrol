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


#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>

#include "../usb k8055/k8055.h"

static int auth = 0;
static int encryption = 0;
static int secure = 0;
static int master = 0;
static int linger = 0;

#define MAXPATHLEN 255
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
	int sk, nsk, fd, lm, ntry = 30;

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

	K8055 *platine=NULL;
	try {
		platine=new K8055(1,false);
		printf("init platine\n");
		platine->write_output(0, 128, 255);
	}catch(const char *errormsg) {
		fprintf(stderr,"ansteuer platine error: %s\n",errormsg);
	}

	printf("Waiting for connection on channel %d\n", laddr.rc_channel);

	listen(sk, 10);

	alen = sizeof(raddr);
	nsk = accept(sk, (struct sockaddr *) &raddr, &alen);

	alen = sizeof(laddr);
	if (getsockname(nsk, (struct sockaddr *)&laddr, &alen) < 0) {
		perror("Can't get RFCOMM socket name");
		close(nsk);
		return;
	}
	printf("socket accepted sending welcome msg\n");
	write(nsk,"hallo\n",6);

	ba2str(&req.dst, dst);
	printf("Connection from %s to %s\n", dst, devname);


	int x=0;
	int speed=0;
	while(1) {
		char buffer[256];
		int size;
		if((size = read(nsk, buffer, sizeof(buffer))) <= 0) {
			printf("error reading bla\n");
			break;
		}
		buffer[size]='\0';
		printf("%.*s",size,buffer);
		if(size != write(nsk,buffer,size)) {
			printf("error writing bla\n");
			break;
		}
		
		int nr=0;
		char cmd[sizeof(buffer)]="";
		sscanf(buffer,"%d %s",&nr,&cmd);
		printf("cmd=%s\n",cmd);
		if(cmd) {
			if(memcmp(cmd,"up",2)==0) {
				speed+=10;
				if(speed > 255)
					speed=255;
			}
			if(memcmp(cmd,"down",4)==0) {
				speed-=10;
				if(speed < 0)
					speed=0;
			}
		} else {
			printf("no command?????");
		}
		if(platine) {
			int ia1=speed;
			int ia2=0;
			int id8=0;
			printf("speed=%d: ",speed);
			for(int i=1; i < 9; i++) {
				printf("%d ",255*i/(9));
				if( speed >= 255*i/(9)) {
					id8 |= 1 << (i-1);
				}
			}
			printf("\n");
			if(speed==0) { // lauflicht anzeigen
				int a=1 << (nr&3);
				id8=a | a << 4;
			}
			platine->write_output(ia1, ia2, id8);
		}

		/*
		if((x++)%10==0) {
			write(nsk,"101010\n",7);
		}
		*/
	}

	if(platine) {
		delete platine;
	}


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
	close(nsk);

	printf("Press CTRL-C for hangup\n");

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
	sigaction(SIGINT,  &sa, NULL); */

	printf("Disconnected\n");

	close(fd);
	return;

release:
	memset(&req, 0, sizeof(req));
	req.dev_id = dev;
	req.flags = (1 << RFCOMM_HANGUP_NOW);
	ioctl(ctl, RFCOMMRELEASEDEV, &req);

	close(sk);
}

int main(int argc, char *argv[])
{
	printf("dudl\n");
	bdaddr_t bdaddr;
	bacpy(&bdaddr, BDADDR_ANY);

	int ctl=1000;
	cmd_listen(ctl, 30, &bdaddr, 30);

}
