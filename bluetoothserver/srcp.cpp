/*
 *  This file is part of btcontrol
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
 * interface zum srcpd
 */
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

bool SRCP::powered=false;


SRCPReply::SRCPReply(const char *message)
{
	// printf("SRCP scan message: %s\n",message);
	sscanf(message,"%lf %d %a[^\n]", &this->timestamp, &this->code, &this->message); 
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
	struct hostent *ServerHost;
	struct sockaddr_in socketAddr;
	const int SERVMSGLEN=1000;
	char servermsg[SERVMSGLEN];
	bzero(&socketAddr,sizeof(socketAddr));

	ServerHost=gethostbyname(cfg_hostname);
	socketAddr.sin_family=ServerHost->h_addrtype;
	memcpy(&socketAddr.sin_addr,ServerHost->h_addr_list[0],sizeof(socketAddr.sin_addr)); 
	socketAddr.sin_port= htons(cfg_port);
	if ((so = socket(AF_INET, SOCK_STREAM, 0 )) == -1) {
		perror("socket()");
		exit(1);
	}
	if ( connect(so,  (struct sockaddr *)&socketAddr, sizeof(socketAddr))<0  ) {
		fprintf(stderr,"\nDaemon srcpd not found on host %s on port %d.\n", 
				cfg_hostname, cfg_port);
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
	this->powered=true;
}

void SRCP::pwrOff()
{
	SRCPReplyPtr reply = this->sendMessage("SET 1 POWER OFF");
	if(reply->type != SRCPReply::OK) {
		fprintf(stderr,"error power on (%s)\n",reply->message);
	}
	printf("STOPVOLTAGE done\n");
	this->powered=false;
}

/**
 * sendet eine message an den SRCPD
 * @return status message, no need to delete()
 */
SRCPReplyPtr SRCP::sendMessage(const char *message)
{
	printf("SRCP send: %s\n",message);
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

	// printf("SRCP reply: %s\n",buffer);
	return SRCPReplyPtr(new SRCPReply(buffer));
}

SRCPReplyPtr SRCP::sendLocoInit(int addr, int nFahrstufen, int nFunc)
{
	char cmd[1024];
	snprintf(cmd,sizeof(cmd),"INIT 1 GL "/* addr:*/ "%d " /* proto:*/ "%c " /* protoversion:*/ "%d " 
		/* nFahrstufen:*/ "%d " /* nFunc:*/ "%d",
		addr, 'N', addr < 128 ? 1 : 2,
		// addr, 'N', addr < 128 ? 1 : 4,
		nFahrstufen, nFunc);
	// printf("sending init: %s\n",cmd);
	return this->sendMessage(cmd);
}

SRCPReplyPtr SRCP::sendLocoSpeed(int addr, int dir, int nFahrstufen, int speed, int nFunc, bool *func)
{
	/* - für srcp 0.7
	const char *proto=NULL;
	switch(nFahrstufen) {
		case 14: proto = "NB"; break;
		case 28: proto = addr < 128 ? "N1" : "N3"; break;
		case 127: proto = addr < 128 ? "N2" : "N4"; break;
		default: assert(nFahrstufen == 14 || nFahrstufen == 28 || nFahrstufen == 127);
	} */
	const int CMDBUFLEN=256;
	char buf[CMDBUFLEN];
	snprintf(buf,CMDBUFLEN-1,"SET 1 GL " /* addr:*/ "%d " /* dir:*/ "%d " /* Fahrstufe:*/ "%d " /* nFahrstufen*/ "%d ",
			addr,
			dir,
			speed,
			nFahrstufen );
	for(int i=0; i < nFunc; i++) {
		int pos=strlen(buf);
		snprintf(buf+pos,CMDBUFLEN-1-pos," %d",func[i]); 
	}
	// printf("cmd: %s\n",buf);
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
	// printf("cmd: %s\n",buf);
	return this->sendMessage(buf);
	
}

SRCPReplyPtr SRCP::sendPOMBit(int addr, int cv, int bitNr, bool value)
{
	const int CMDBUFLEN=256;
	char buf[CMDBUFLEN];
	snprintf(buf,CMDBUFLEN-1,"SET 1 SM " /* addr:*/ "%d CVBIT " /* cv# */ "%d " /* bit# */ "%d " /* value */ "%d",
		addr,
		cv,
		bitNr,
		value);
	// printf("cmd: %s\n",buf);
	return this->sendMessage(buf);
	
}

/**
 * fragt den status (speed,func) einer lok ab TODO: fertig machen, bringt nach einem init nix, lok ist auf 000000 resettet ...
 * @param func muss nFunc platz haben 
 */
bool SRCP::getInfo(int addr, int *dir, int *dccSpeed, int nFunc, bool *func)
{
	const int CMDBUFLEN=256;
	char buf[CMDBUFLEN];
	snprintf(buf,CMDBUFLEN-1,"GET 1 GL " /* addr:*/ "%d",
		addr);
	// printf("cmd: %s\n",buf);
	SRCPReplyPtr reply = this->sendMessage(buf);
	if(reply->type == SRCPReply::INFO) {
		printf("scanning reply: %s\n",reply->message);
		int saddr, sdrivemode, sv, svmax;
		// INFO 1 GL 647 0 0 128 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		int rc=sscanf(reply->message,"%*s 1 GL " /*addr*/ "%d " /*drivemode*/ "%d " /* V */ "%d " /* V_max */ "%d " /* F0 .. Fn */,
			&saddr,
			&sdrivemode,
			&sv,
			&svmax);
		printf("printf matched %d vars, v=%d vmax=%d\n",rc, sv, svmax);
//	sscanf(message,"%lf %d %a[^\n]", &this->timestamp, &this->code, &this->message); 
		return true;
	}
	return false;
}
