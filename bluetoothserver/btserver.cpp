#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>

#include "server.h"


static int auth = 0;
static int encryption = 0;
static int secure = 0;
static int master = 1;
static int linger = 0;

/**
 *
 *
 *
 * mehr oder weniger cmd_listen aus */
//  http://bluez.cvs.sourceforge.net/*checkout*/bluez/utils/rfcomm/main.c?revision=1.31
BTServer::BTServer(int rc_channel)
{
	struct sockaddr_rc laddr;
	// int ctl = ???
	// int dev = /dev/rfcomm%d
//	struct termios ti;
//	struct sigaction sa;
//	struct pollfd p;
	sigset_t sigs;
	socklen_t alen;
	// , devname[MAXPATHLEN]="";
	int lm, ntry = 30;

	laddr.rc_family = AF_BLUETOOTH;
	bacpy(&laddr.rc_bdaddr, BDADDR_ANY);
	laddr.rc_channel = rc_channel;

	this->so = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if (this->so < 0) {
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

	if (lm && setsockopt(this->so, SOL_RFCOMM, RFCOMM_LM, &lm, sizeof(lm)) < 0) {
		perror("Can't set RFCOMM link mode");
		close(this->so);
		this->so=0;
		return;
	}

	if (bind(this->so, (struct sockaddr *)&laddr, sizeof(laddr)) < 0) {
		perror("Can't bind RFCOMM socket");
		close(this->so);
		this->so=0;
		return;
	}



	listen(this->so, 10);
}

/**
 * wartet auf eine connection
 * @return FD zu einem filehandle, -1 on error
 */
int BTServer::accept()
{
	// struct rfcomm_dev_req req;
	struct sockaddr_rc laddr,raddr;
	char dst[18];

	printf("Waiting for connection ...\n");

	socklen_t addrlen=sizeof(raddr);
	int nsk = ::accept(this->so, (struct sockaddr *) &raddr, &addrlen);

// da könnte die lokale bt addr drinnen stehn
	if (getsockname(nsk, (struct sockaddr *)&laddr, &addrlen) < 0) {
		perror("Can't get RFCOMM socket name");
		close(nsk);
		return -1;
	}

/*
	memset(&req, 0, sizeof(req));
	bacpy(&req.src, &laddr.rc_bdaddr);
	bacpy(&req.dst, &raddr.rc_bdaddr);
	req.channel = raddr.rc_channel;
*/
	ba2str(&raddr.rc_bdaddr, dst);
	printf("Connection from %s\n", dst);
	return nsk;

}


BTServer::~BTServer()
{

	printf("Disconnected\n");
	close(this->so);

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



/*
	close(sk);

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

/*
release:
	memset(&req, 0, sizeof(req));
	req.dev_id = dev;
	req.flags = (1 << RFCOMM_HANGUP_NOW);
	ioctl(ctl, RFCOMMRELEASEDEV, &req);

	close(sk);
*/
}