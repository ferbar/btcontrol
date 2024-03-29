/*
 *  This file is part of btcontrol
 *
 *  Copyright (C) Christian Ferbar
 *
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
#include <errno.h>
#include "srcp.h"
#include "utils.h"
#include "lokdef.h"

#define TAG "SRCP"

bool SRCP::powered=false;


SRCPReply::SRCPReply(const char *message)
{
	// printf("SRCP scan message: %s\n",message);
	// %m modifier: 'allocate' (alt %a)
	sscanf(message,"%lf %d %m[^\n]", &this->timestamp, &this->code, &this->message); 
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
	DEBUGF("SRCPReply: %d %s\n", this->code, this->message);
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
		throw std::runtime_error(utils::format("error creating socket (%s)", strerror(errno)));
	}
	if ( connect(so,  (struct sockaddr *)&socketAddr, sizeof(socketAddr))<0  ) {
		throw std::runtime_error(utils::format("Daemon srcpd not found on host %s on port %d.", cfg_hostname, cfg_port));
	}

	memset(servermsg,0,SERVMSGLEN);
	read(so,servermsg,SERVMSGLEN-1);
	DEBUGF("\nconnected to: %s", servermsg);
	SRCPReplyPtr reply;
	reply=this->sendMessage("SET PROTOCOL SRCP 0.8");
	if(reply->type != SRCPReply::OK) {
		ERRORF("error setting protocol: %s",reply->message);
		abort();
	}
	reply=this->sendMessage("SET CONNECTIONMODE SRCP COMMAND");
	if(reply->type != SRCPReply::OK) {
		ERRORF("error setting protocol: %s",reply->message);
		abort();
	}
	reply=this->sendMessage("GO");
	if(reply->type != SRCPReply::OK) {
		ERRORF("error setting protocol: %s",reply->message);
		abort();
	}
	this->pwrOn();
}

SRCP::~SRCP()
{
	this->pwrOff();
	const char *buf="LOGOUT\n";
	write(so,buf,strlen(buf));
	close(so);
	DEBUGF("STOPVOLTAGE done");
}

void SRCP::pwrOn()
{
	SRCPReplyPtr reply = this->sendMessage("SET 1 POWER ON");
	if(reply->type != SRCPReply::OK) {
		ERRORF("error power on (%s)",reply->message);
	}
	DEBUGF("STARTVOLTAGE done");
	this->powered=true;
}

void SRCP::pwrOff()
{
	SRCPReplyPtr reply = this->sendMessage("SET 1 POWER OFF");
	if(reply->type != SRCPReply::OK) {
		ERRORF("error power on (%s)",reply->message);
	}
	DEBUGF("STOPVOLTAGE done");
	this->powered=false;
}

/**
 * sendet eine message an den SRCPD
 * @return status message, no need to delete()
 */
SRCPReplyPtr SRCP::sendMessage(const char *message)
{
	DEBUGF("SRCP send: %s",message);
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
		ERRORF("error reading reply (%s)",strerror(errno));
		abort();
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
		DEBUGF("scanning reply: %s",reply->message);
		int saddr, sdrivemode, sv, svmax;
		// INFO 1 GL 647 0 0 128 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		int rc=sscanf(reply->message,"%*s 1 GL " /*addr*/ "%d " /*drivemode*/ "%d " /* V */ "%d " /* V_max */ "%d " /* F0 .. Fn */,
			&saddr,
			&sdrivemode,
			&sv,
			&svmax);
		DEBUGF("printf matched %d vars, v=%d vmax=%d",rc, sv, svmax);
//	sscanf(message,"%lf %d %a[^\n]", &this->timestamp, &this->code, &this->message); 
		return true;
	}
	return false;
}

void SRCP_Hardware::fullstop(bool stopAll, bool emergencyStop) {
	int clientID=utils::getThreadClientID();
	int addr_index=0;
	while(lokdef[addr_index].addr) {
		if(clientID)
			DEBUGF("last client [%d]=%d",addr_index,lokdef[addr_index].currspeed);
		if( (stopAll && (lokdef[addr_index].currspeed != 0) ) ||
		    (!stopAll && (lokdef[addr_index].lastClientID == clientID ) ) ) {
			if(clientID)
				DEBUGF("\tstop %d",addr_index);
			else
				DEBUGF("emgstop [%d]=addr:%d\n",addr_index, lokdef[addr_index].addr);
			lokdef[addr_index].currspeed=0;
			this->sendLoco(addr_index, true);
			lokdef[addr_index].lastClientID=0;
		}
		addr_index++;
	}
}

void SRCP_Hardware::sendLoco(int addr_index, bool emergencyStop) {
	int clientID=utils::getThreadClientID();
	int msgNum=utils::getThreadMessageID();
					int dir= lokdef[addr_index].currdir < 0 ? 0 : 1;
					if(emergencyStop) {
						dir=2;
					}
					int nFahrstufen = 128;
					if(lokdef[addr_index].flags & F_DEC14) {
						nFahrstufen = 14;
					} else if(lokdef[addr_index].flags & F_DEC28) {
						nFahrstufen = 28;
					}
					int dccSpeed = abs(lokdef[addr_index].currspeed) * nFahrstufen / 255;
					bool func[MAX_NFUNC];
					for(int j=0; j < lokdef[addr_index].nFunc; j++) {
						func[j]=lokdef[addr_index].func[j].ison;
					}
					if(!lokdef[addr_index].initDone) {
						SRCPReplyPtr replyInit = this->sendLocoInit(lokdef[addr_index].addr, nFahrstufen, lokdef[addr_index].nFunc);
						if(replyInit->type != SRCPReply::OK) {
							ERRORF("%d/%d: error init loco: (%s)", clientID, msgNum, replyInit->message);
							if(replyInit->code == 412) {
								ERRORF("%d/%d: loopback/ddl|number_gl, max addr < %d?", clientID, msgNum, lokdef[addr_index].addr);
								lokdef[addr_index].currspeed=-1;
							}
						} else {
							lokdef[addr_index].initDone=true;
							ERRORF("try to read curr state...");
							if(!this->getInfo(lokdef[addr_index].addr,&dir,&dccSpeed,lokdef[addr_index].nFunc, func)) {
								ERRORF("%d/%d: error getting state of loco: (%s)", clientID, msgNum, replyInit->message);
							}
						}
					}
					SRCPReplyPtr reply = this->sendLocoSpeed(lokdef[addr_index].addr, dir, nFahrstufen, dccSpeed, lokdef[addr_index].nFunc, func);

					if(reply->type != SRCPReply::OK) {
						ERRORF("%d/%d: error sending speed: (%s)", clientID, msgNum, reply->message);
					}
}

int SRCP_Hardware::sendPOM(int addr, int cv, int value) {
	if(cv < 0) {
		NOTICEF("SRCP::sendPOM(addr=%d, cv=%d)", addr, cv);
		return -1;
	}
	return SRCP::sendPOM(addr, cv, value)->type != SRCPReply::OK ? -1 : value ;
}
