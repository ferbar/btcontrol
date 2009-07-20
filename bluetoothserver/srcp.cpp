#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "srcp.h"
#include <errno.h>



SRCPReply::SRCPReply(const char *message)
{
	sscanf(message,"%f %d %*s %a[^\n]", &this->timestamp, &this->code, &this->message); 
	if(this->code >= 500)
		this->type=SRCPReply::ERROR;
	else if(this->code >= 400)
		this->type=SRCPReply::ERROR;
	else if(this->code >= 200)
		this->type=SRCPReply::OK;
	else if(this->code >= 100)
		this->type=SRCPReply::INFO;
	else
		this->type=SRCPReply::ERROR;
	printf("SRCPReply: %d %s\n", this->code, this->message);
}

SRCPReply::~SRCPReply()
{
	if(this->message) {
		free(this->message);
		this->message=(char*)-1;
	}
}


SRCP::SRCP()
{
	int port=4303;
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
		fprintf(stderr,"\nDaemon srcpd not found on host %s on port %d.\n", 
				hostName, port);
		fprintf(stderr,"srcpd is not running or is running on another port.\n\n");
		exit(1);
	}

	memset(servermsg,0,SERVMSGLEN);
	read(so,servermsg,SERVMSGLEN-1);
	printf("\nconnected to: %s\n", servermsg);
	SRCPReplyPtr reply;
	reply=this->sendMessage("SET PROTOCOL SRCP 0.8");
	if(reply->type != SRCPReply::OK) {
		fprintf(stderr,"error setting protocol: %s\n",reply->message);
		exit(1);
	}
	reply=this->sendMessage("SET CONNECTIONMODE SRCP COMMAND");
	if(reply->type != SRCPReply::OK) {
		fprintf(stderr,"error setting protocol: %s\n",reply->message);
		exit(1);
	}
	reply=this->sendMessage("GO");
	if(reply->type != SRCPReply::OK) {
		fprintf(stderr,"error setting protocol: %s\n",reply->message);
		exit(1);
	}
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
	SRCPReplyPtr reply = this->sendMessage("SET 1 POWER ON");
	if(reply->type != SRCPReply::OK) {
		fprintf(stderr,"error power on (%s)\n",reply->message);
	}
	printf("STARTVOLTAGE done\n");
}

void SRCP::pwrOff()
{
	SRCPReplyPtr reply = this->sendMessage("SET 1 POWER OFF");
	if(reply->type != SRCPReply::OK) {
		fprintf(stderr,"error power on (%s)\n",reply->message);
	}
	printf("STOPVOLTAGE done\n");
}

/**
 * sendet eine message an den SRCPD
 * @return status message, no need to delete()
 */
SRCPReplyPtr SRCP::sendMessage(const char *message)
{
	write(this->so,message,strlen(message));
	write(this->so,"\n",1);
	return this->readReply();
}

SRCPReplyPtr SRCP::readReply()
{
	char buffer[1024];
	int n=read(this->so,buffer,sizeof(buffer));
	if(n <= 0) {
		// exception ???
		fprintf(stderr,"error reading reply (%s)\n",strerror(errno));
		exit(1);
	}
	buffer[n]=0;

	return SRCPReplyPtr(new SRCPReply(buffer));
}

SRCPReplyPtr SRCP::sendLocoInit(int addr, int nFahrstufen, int nFunc)
{
	char cmd[1024];
	snprintf(cmd,sizeof(cmd),"INIT 1 GL "/* addr:*/ "%d " /* proto:*/ "%c " /* protoversion:*/ "%d " 
		/* nFahrstufen:*/ "%d " /* nFunc:*/ "%d",
		addr, 'N', addr < 128 ? 1 : 2,
		nFahrstufen, nFunc+1);
	printf("sending init: %s\n",cmd);
	return this->sendMessage(cmd);
}

SRCPReplyPtr SRCP::sendLocoSpeed(int addr, int dir, int nFahrstufen, int speed, int nFunc, bool *func)
{
	// const char *proto = addr > 32 ? "N2" : "N1";
	const char *proto=NULL;
	switch(nFahrstufen) {
		case 14: proto = "NB"; break;
		case 28: proto = addr < 128 ? "N1" : "N3"; break;
		case 127: proto = addr < 128 ? "N2" : "N4"; break;
		default: assert(nFahrstufen == 14 || nFahrstufen == 28 || nFahrstufen == 127);
	}
	const int CMDBUFLEN=256;
	char buf[CMDBUFLEN];
	snprintf(buf,CMDBUFLEN-1,"SET 1 GL " /* addr:*/ "%d " /* dir:*/ "%d " /* Fahrstufe:*/ "%d " /* nFahrstufen*/ "%d " /* F0:*/ "%d ",
			addr,
			dir,
			speed,
			nFahrstufen,
			1,
			nFunc);
	for(int i=0; i < nFunc; i++) {
		int pos=strlen(buf);
		snprintf(buf+pos,CMDBUFLEN-1-pos," %d",func[i]); 
	}
	printf("cmd: %s", buf);
	return this->sendMessage(buf);
}

SRCPReplyPtr SRCP::sendPOM(int addr, int cv, int value)
{
	const int CMDBUFLEN=256;
	char buf[CMDBUFLEN];
	snprintf(buf,CMDBUFLEN-1,"SET 1 SM " /* addr:*/ "%d CV " /* cv# */ "%d " /* value */ "%d",
		addr,
		cv,
		value);
	printf("cmd: %s\n",buf);
	return this->sendMessage(buf);
	
}
