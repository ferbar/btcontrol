#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "srcp.h"

SRCP::SRCP()
{
	int port=12345;
	const char *hostName="127.0.0.1";
	struct hostent *ServerHost;
	struct sockaddr_in socketAddr;
	const int SERVMSGLEN=1000;
	char servermsg[SERVMSGLEN];
	bzero(&socketAddr,sizeof(socketAddr));

	ServerHost=gethostbyname(hostName);
	socketAddr.sin_family=ServerHost->h_addrtype;
	memcpy(&socketAddr.sin_addr,ServerHost->h_addr_list[0],sizeof(socketAddr.sin_addr)); 
	socketAddr.sin_port= htons(port);
	if ((so = socket(AF_INET, SOCK_STREAM, 0 )) == -1) {
		perror("socket()");
		exit(1);
	}
	if ( connect(so,  (struct sockaddr *)&socketAddr, sizeof(socketAddr))<0  ) {
		fprintf(stderr,"\nDaemon erddcd not found on host %s on port %d.\n", 
				hostName, port);
		fprintf(stderr,"erddcd is not running or is running on another port.\n\n");
		exit(1);
	}

	memset(servermsg,0,SERVMSGLEN);
	read(so,servermsg,SERVMSGLEN-1);
	printf("\nconnected to: %s\n", servermsg);
	this->pwrOn();
}

SRCP::~SRCP()
{
	this->pwrOff();
	const char *buf="LOGOUT\n";
	write(so,buf,strlen(buf));
	close(so);
	printf("STOPVOLTAGE done\n");
}

void SRCP::pwrOn()
{
	const char *buf="STARTVOLTAGE\n";
	if(write(so,buf,strlen(buf)) != strlen(buf)) {
		printf("error writing start command!\n");
	}
	printf("STARTVOLTAGE done\n");
}

void SRCP::pwrOff()
{
	const char *buf="STOPVOLTAGE\n";
	if(write(so,buf,strlen(buf)) != strlen(buf)) {
		printf("error writing start command!\n");
	}
	printf("STOPVOLTAGE done\n");
}

void SRCP::send(int addr, int dir, int nFahrstufen, int speed, int nFunc, bool *func)
{
	assert(nFunc==4);
	const char *proto = addr > 32 ? "N2" : "N1";
	const int CMDBUFLEN=256;
	char buf[CMDBUFLEN];
	snprintf(buf,CMDBUFLEN-1,"SET GL %s %d %d %d " /* nFahrstufen*/ "%d " /* licht ein:*/ "%d " /*nFunc:*/ "%d %d %d %d %d\n",
			proto,
			addr,
			dir,
			speed,
			nFahrstufen,
			1,
			nFunc,
			func[0],func[1],
			func[2],func[3]);
	printf("cmp: %s", buf);
	write(so,buf,strlen(buf));

}
