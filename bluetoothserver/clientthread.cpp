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
 *
 * client - thread - part (für jeden client ein thread)
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <assert.h>
#include "clientthread.h"
#include "lokdef.h"
#include "srcp.h"
#include "USBPlatine.h"
#include <stdexcept>

// für setsockopt
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include <errno.h>

#include "utils.h"
#include "server.h"

#ifdef HAVE_ALSA
#include "sound.h"
#endif

extern USBPlatine *platine;

int ClientThread::numClients=0;

#define sendToPhone(text) \
		if(strlen(text) != write(startupdata->so,text,strlen(text))) { \
			printf("%d:error writing message (%s)\n",startupdata->clientID, strerror(errno)); \
			break; \
		} else { \
			printf("%d: ->%s",startupdata->clientID,text); \
		}


void ClientThread::sendMessage(const FBTCtlMessage &msg)
{
	std::string binMsg=msg.getBinaryMessage();
	int msgsize=binMsg.size();
	printf("%d:  sendMessage size: %zu+4 %d=%s\n", this->clientID, binMsg.size(), msg.getType(), messageTypeName(msg.getType()).c_str());
	int flag = 1;
	setsockopt(this->so, IPPROTO_TCP, TCP_CORK, (char *)&flag, sizeof(flag) ); // prepare message, stopsel rein
	write(this->so, &msgsize, 4);
	write(this->so, binMsg.data(), binMsg.size());
	flag=0;
	setsockopt(this->so, IPPROTO_TCP, TCP_CORK, (char *)&flag, sizeof(flag) ); // message fertig, senden
}

/**
 * wartet bis daten daherkommen, macht exception wenn keine innerhalb von timeout gekommen sind
 */
void ClientThread::readSelect()
{
	struct timeval timeout;
	fd_set set;
	timeout.tv_sec=cfg_tcpTimeout; timeout.tv_usec=0;
	FD_ZERO(&set); FD_SET(this->so,&set);
	int rc;
	if((rc=select(this->so+1, &set, NULL, NULL, &timeout)) <= 0) {
		if(rc != 0) {
			throw std::runtime_error("error select");
		}
		printf("ClientThread::readSelect error in select(%d) %s\n", this->so, strerror(errno));
		throw std::runtime_error("timeout reading cmd");
	}
}

void ClientThread::setLokStatus(FBTCtlMessage &reply, lastStatus_t *lastStatus)
{
	// FBTCtlMessage test(messageTypeID("PING_REPLY"));
	int n=0;
	int i=0;
	reply["info"]=FBTCtlMessage(MessageLayout::ARRAY); // feld anlegen damits immer da ist
	while(lokdef[i].addr) {
		int func=0;
		for(int j=0; j < lokdef[i].nFunc; j++) {
			if(lokdef[i].func[j].ison)
				func |= 1 << j;
		}
		if((lokdef[i].currspeed != lastStatus[i].currspeed) || (func != lastStatus[i].func) || 
			(lokdef[i].currdir != lastStatus[i].dir)) {
			lastStatus[i].currspeed=lokdef[i].currspeed;
			lastStatus[i].func=func;
			lastStatus[i].dir=lokdef[i].currdir;
			reply["info"][n]["addr"]=lokdef[i].addr;
			int speed=lokdef[i].currspeed;
			int dir=lokdef[i].currdir;
			if(speed==0) {
				reply["info"][n]["speed"]=dir;
			} else {
				reply["info"][n]["speed"]=speed*dir;
			}
			reply["info"][n]["functions"]=func;
			n++;
		}
		i++;
	}
}

void ClientThread::sendStatusReply(lastStatus_t *lastStatus)
{
	FBTCtlMessage reply(messageTypeID("STATUS_REPLY"));
	setLokStatus(reply,lastStatus);
	// reply.dump();
	sendMessage(reply);
}

/**
 * prog an ein handy senden
 *
 */
void ClientThread::sendClientUpdate()
{
#ifdef INCL_BT
	BTServer::pushUpdate(this->so);
#else
	printf("ClientThread::sendClientUpdate ohne BT\n");
	abort();
#endif
}

/**
 * für jedes handy ein eigener thread...
 */
void ClientThread::run()
{
	// startupdata_t *startupdata=(startupdata_t *)data;

#ifdef HAVE_ALSA
	FahrSound sound(cfg_soundFiles);
	if(platine && FahrSound::soundFiles != NULL) { // nur wenn eine platine angeschlossen und sound files geladen
		sound.init();
		sound.run();
	}
#endif
	printf("%d:socket accepted sending welcome msg\n",this->clientID);
	FBTCtlMessage heloReply(messageTypeID("HELO"));
	heloReply["name"]="my bt server";
	heloReply["version"]="0.9";
	heloReply["protohash"]=messageLayouts.protocolHash;
	// heloReply.dump();
	sendMessage(heloReply);
	heloReply.clear(); // brauch ma nachher nichtmehr

	int nLokdef=0;
	while(lokdef[nLokdef].addr) nLokdef++;
	lastStatus_t lastStatus[nLokdef+1];
	bzero(lastStatus, sizeof(lastStatus));
	for(int i=0; i <= nLokdef; i++) { lastStatus[i].func=-1; }
	bool changedAddrIndex[nLokdef+1];
	for(int i=0; i <= nLokdef; i++) { changedAddrIndex[i] = false; }


	printf("%d:hello done, enter main loop\n",this->clientID);
	// int speed=0;
	// int addr=3;
// int sleepTime=0; -> velleman platine test
	while(1) {
		// addr_index sollte immer gültig sin - sonst gibts an segv ....
/* velleman platine antwortet bei einem command abstand von < 1s immer brav, 1-2s wartets immer bis zur vollen sekunde, >2s passt meistens
if(sleepTime <= 0) {
	sleepTime= 3000000;
}
printf("sleep: %d\n",sleepTime);
usleep(sleepTime);
sleepTime-=90000;
platine->onebench();
continue;
*/

		bool emergencyStop=false;

		int msgsize=0;
		int rc;
		/*
		struct timeval t,t0;
		gettimeofday(&t0, NULL);
		*/
		this->readSelect(); // auf daten warten, macht exception wenn innerhalb vom timeout nix kommt
		if((rc=read(this->so, &msgsize, 4)) != 4) {
			throw std::runtime_error("error reading cmd: " + rc);
		}
		/*
		gettimeofday(&t, NULL);
		int us=(t.tv_sec - t0.tv_sec) * 1000000 + t.tv_usec - t0.tv_usec;
		printf("select + read in %dµs\n",us);
		*/
		// printf("%d:reading msg.size: %d bytes\n",this->clientID,msgsize);
		if(msgsize < 0 || msgsize > 10000) {
			throw std::runtime_error("invalid size msgsize 2big");
		}
		char buffer[msgsize];
		this->readSelect();
		if((rc=read(this->so, buffer, msgsize)) != msgsize) {
			throw std::runtime_error("error reading cmd.data: " + rc );
		}
		InputReader in(buffer,msgsize);
		// printf("%d:parsing msg\n",this->clientID);
		FBTCtlMessage cmd;
		cmd.readMessage(in);
		if(cfg_debug) {
			printf("%d/%d: msg",this->clientID, this->msgNum);
			cmd.dump();
		} else {
			printf("%d/%d: msg %d=%s\n", this->clientID, this->msgNum, cmd.getType(), messageTypeName(cmd.getType()).c_str());
		}
		/*
		int nr=0; // für die conrad platine
		int size;
		char buffer[256];
		if((size = read(startupdata->so, buffer, sizeof(buffer))) <= 0) {
			printf("%d:error reading message size=%d \"%.*s\"\n",startupdata->clientID,size,size >= 0 ? buffer : NULL);
			break;
		}
		buffer[size]='\0';
		printf("%.*s",size,buffer);
		char cmd[sizeof(buffer)]="";
		char param1[sizeof(buffer)]="";
		char param2[sizeof(buffer)]="";
		sscanf(buffer,"%d %s %s %s",&nr,&cmd, &param1, &param2);
		printf("%d:<- %s p1=%s p2=%s\n",startupdata->clientID,cmd,param1,param2);
		*/
		if(true) {
			if(cmd.isType("PING")) {
				sendStatusReply(lastStatus);
			} else if(cmd.isType("ACC")) {
				int addr=cmd["addr"].getIntVal();
				int addr_index=getAddrIndex(addr);
				lokdef[addr_index].currspeed+=5;
				if(lokdef[addr_index].currspeed > 255)
					lokdef[addr_index].currspeed=255;
				sendStatusReply(lastStatus);
				changedAddrIndex[addr_index]=true;
			} else if(cmd.isType("BREAK")) {
				int addr=cmd["addr"].getIntVal();
				int addr_index=getAddrIndex(addr);
				lokdef[addr_index].currspeed-=5;
				if(lokdef[addr_index].currspeed < 0)
					lokdef[addr_index].currspeed=0;
				sendStatusReply(lastStatus);
				changedAddrIndex[addr_index]=true;
			} else if(cmd.isType("DIR")) {
				int addr=cmd["addr"].getIntVal();
				int addr_index=getAddrIndex(addr);
				int dir=cmd["dir"].getIntVal();
				if(dir != 1 && dir != -1) {
					throw std::runtime_error("invalid dir");
				}
				if(lokdef[addr_index].currspeed != 0) {
					throw std::runtime_error("speed != 0");
				}
				lokdef[addr_index].currdir=dir;
				sendStatusReply(lastStatus);
				changedAddrIndex[addr_index]=true;
			} else if(cmd.isType("STOP")) {
				int addr=cmd["addr"].getIntVal();
				int addr_index=getAddrIndex(addr);
				lokdef[addr_index].currspeed=0;
				sendStatusReply(lastStatus);
				changedAddrIndex[addr_index]=true;
			} else if(cmd.isType("ACC_MULTI")) {
				int n=cmd["list"].getArraySize();
				int addr=cmd["list"][0]["addr"].getIntVal();
				int addr_index=getAddrIndex(addr);
				lokdef[addr_index].currspeed+=5;
				if(lokdef[addr_index].currspeed > 255)
					lokdef[addr_index].currspeed=255;
				changedAddrIndex[addr_index]=true;
				int speed=lokdef[addr_index].currspeed;
				for(int i=1; i < n; i++) {
					int addr=cmd["list"][i]["addr"].getIntVal();
					int addr_index=getAddrIndex(addr);
					lokdef[addr_index].currspeed=speed;
					changedAddrIndex[addr_index]=true;
				}
				sendStatusReply(lastStatus);
			} else if(cmd.isType("BREAK_MULTI")) {
				int n=cmd["list"].getArraySize();
				int addr=cmd["list"][0]["addr"].getIntVal();
				int addr_index=getAddrIndex(addr);
				lokdef[addr_index].currspeed-=5;
				if(lokdef[addr_index].currspeed < -255)
					lokdef[addr_index].currspeed=-255;
				changedAddrIndex[addr_index]=true;
				int speed=lokdef[addr_index].currspeed;
				for(int i=1; i < n; i++) {
					int addr=cmd["list"][i]["addr"].getIntVal();
					int addr_index=getAddrIndex(addr);
					lokdef[addr_index].currspeed=speed;
					changedAddrIndex[addr_index]=true;
				}
				sendStatusReply(lastStatus);
			} else if(cmd.isType("DIR_MULTI")) {
				// cmd.dump();
				int n=cmd["list"].getArraySize();
				int dir=cmd["dir"].getIntVal();
				for(int i=0; i < n; i++) {
					int addr=cmd["list"][i]["addr"].getIntVal();
					int addr_index=getAddrIndex(addr);
					lokdef[addr_index].currdir=dir;
					changedAddrIndex[addr_index]=true;
				}
				sendStatusReply(lastStatus);
			} else if(cmd.isType("STOP_MULTI")) {
				int n=cmd["list"].getArraySize();
				for(int i=0; i < n; i++) {
					int addr=cmd["list"][i]["addr"].getIntVal();
					int addr_index=getAddrIndex(addr);
					lokdef[addr_index].currspeed=0;
					changedAddrIndex[addr_index]=true;
				}
				sendStatusReply(lastStatus);
			} else if(cmd.isType("SETFUNC")) {
				int addr=cmd["addr"].getIntVal();
				int addr_index=getAddrIndex(addr);
				int funcNr=cmd["funcnr"].getIntVal();
				int value=cmd["value"].getIntVal();
				if(funcNr >= 0 && funcNr < lokdef[addr_index].nFunc) {
					if(cfg_debug) printf("%d/%d:set funcNr[%d]=%d\n",this->clientID,this->msgNum,funcNr,value);
					if(value)
						lokdef[addr_index].func[funcNr].ison = true;
					else
						lokdef[addr_index].func[funcNr].ison = false;
				} else {
					printf(ANSI_RED "%d/%d:invalid funcNr out of bounds(%d)\n" ANSI_DEFAULT, this->clientID,this->msgNum,funcNr);
				}
				sendStatusReply(lastStatus);
				changedAddrIndex[addr_index]=true;

/*
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
*/
			} else if(cmd.isType("GETLOCOS")) { // liste mit eingetragenen loks abrufen, format: <name>;<adresse>;...\n
				FBTCtlMessage reply(messageTypeID("GETLOCOS_REPLY"));
				int i=0;
				while(lokdef[i].addr) {
					reply["info"][i]["addr"]=lokdef[i].addr;
					reply["info"][i]["name"]=lokdef[i].name;
					reply["info"][i]["imgname"]=lokdef[i].imgname;
					if(lokdef[i].currspeed==0) {
						reply["info"][i]["speed"]=lokdef[i].currdir;
					} else {
						reply["info"][i]["speed"]=lokdef[i].currspeed * lokdef[i].currdir;
					}
					int func=0;
					for(int j=0; j < lokdef[i].nFunc; j++) {
						if(lokdef[i].func[j].ison)
							func |= 1 << j;
					}
					reply["info"][i]["functions"]=func;
					i++;
				}
				// reply.dump();
				sendMessage(reply);
			} else if(cmd.isType("GETFUNCTIONS")) {
				int addr=cmd["addr"].getIntVal();
				int addr_index=getAddrIndex(addr);
				FBTCtlMessage reply(messageTypeID("GETFUNCTIONS_REPLY"));
				printf("%d/%d: funclist for addr %d\n",this->clientID,this->msgNum,addr);
				// char buffer[32];
				for(int i=0; i < lokdef[addr_index].nFunc; i++) {
					// snprintf(buffer,sizeof(buffer)," %d;%d;%s\n",j+1,lokdef[addr_index].func[j].ison,lokdef[addr_index].func[j].name);
					reply["info"][i]["name"]=lokdef[addr_index].func[i].name;
					reply["info"][i]["value"]=lokdef[addr_index].func[i].ison;
					reply["info"][i]["imgname"]="";
				}
				sendMessage(reply);
			} else if(cmd.isType("POWER")) { // special: value=-1   -> nix ändern, nur status liefern
// TODO: alle loks auf speed=0 setzten, dir=notaus
				int value=cmd["value"].getIntVal();
				if(srcp && (value != -1) ) {
					if(value) srcp->pwrOn();
					else srcp->pwrOff();
				}
				FBTCtlMessage reply(messageTypeID("POWER_REPLY"));
				reply["value"]=srcp->powered;
				sendMessage(reply);
			} else if(cmd.isType("POM")) {
				int addr=cmd["addr"].getIntVal();
				int addr_index=getAddrIndex(addr);
				int cv=cmd["cv"].getIntVal();
				int value=cmd["value"].getIntVal();
				FBTCtlMessage reply(messageTypeID("POM_REPLY"));
				if(srcp) {
					reply["value"]=1;
					srcp->sendPOM(lokdef[addr_index].addr, cv, value);
				} else {
				#ifdef HAVE_ALSA
					if(cv==266) {
					    sound.setMasterVolume(value);
						reply["value"]=1;
					} else {
				#endif
						reply["value"]=0;
				#ifdef HAVE_ALSA
					}
				#endif
				}
				sendMessage(reply);
			} else if(cmd.isType("POMBIT")) {
				int addr=cmd["addr"].getIntVal();
				int addr_index=getAddrIndex(addr);
				int cv=cmd["cv"].getIntVal();
				int bitNr=cmd["bit"].getIntVal();
				int value=cmd["value"].getIntVal();
				FBTCtlMessage reply(messageTypeID("POMBIT_REPLY"));
				if(srcp) {
					reply["value"]=1;
					srcp->sendPOMBit(lokdef[addr_index].addr, cv, bitNr, value);
				} else {
					reply["value"]=0;
				}
				sendMessage(reply);
			} else if(cmd.isType("GETIMAGE")) {
				std::string imageName=cmd["imgname"].getStringVal();
				FBTCtlMessage reply(messageTypeID("GETIMAGE_REPLY"));
				reply["img"]=readFile("img/"+imageName);
				sendMessage(reply);
			} else if(cmd.isType("HELO_ERR")) {
				printf(ANSI_RED "%d/%d: client proto error\n" ANSI_DEFAULT, this->clientID, this->msgNum);
				int protohash=cmd["protohash"].getIntVal();
				int doupdate=cmd["doupdate"].getIntVal();
				printf("hash=%d (me:%d), doupdate=%d\n",protohash,messageLayouts.protocolHash,doupdate);
				this->sendClientUpdate();
#ifdef INCL_BT
			} else if(cmd.isType("BTSCAN")) { // liste mit eingetragenen loks abrufen, format: <name>;<adresse>;...\n
				FBTCtlMessage reply(messageTypeID("BTSCAN_REPLY"));
				BTServer::BTScan(reply);
				// reply.dump();
				sendMessage(reply);
			} else if(cmd.isType("BTPUSH")) { // 
				printf("BTPUSH ---------------------------------------------------\n");
				FBTCtlMessage reply(messageTypeID("BTPUSH_REPLY"));
				std::string addr=cmd["addr"].getStringVal();
				// TODO: ussppush oder gammu push 
				// int type=cmd["type"].getIntVal();
				BTServer::BTPush(addr);

				// reply.dump();
				reply["rc"]=1;
				sendMessage(reply);
#endif
			} else {
				printf(ANSI_RED "%d/%d:----------------- invalid/unimplemented command (%d,%s)------------------------\n" ANSI_DEFAULT,
					this->clientID,this->msgNum,cmd.getType(),messageTypeName(cmd.getType()).c_str());
			/*
			if(memcmp(cmd,"invalid_key",10)==0) {
				printf("%d:invalid key ! param1: %s\n",startupdata->clientID,param1);
				bool notaus=false;
				/ *
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
				* /
				if(notaus) {
					printf("notaus\n");
					lokdef[addr_index].currspeed=0;
					emergencyStop=true;
				}
				*/
			}
		} else {
			printf("%d:no command?????",this->clientID);
		}

		/*
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
		*/

		// PLATINE ANSTEUERN ------------
		if(platine) {
			// geschwindigkeit 
			// double f_speed=sqrt(sqrt((double)lokdef[addr_index].currspeed/255.0))*255.0; // für üperhaupt keine elektronik vorm motor gut (schienentraktor)
			// TODO: wemmas wieder brauchen sollt gucken ob sich wirklich was geändert hat
			// int changedAddr=3; // eine adresse mit der nummer muss in der lokdef eingetragen sein
			int addr_index=-1; // getAddrIndex(changedAddr);
			for(int i=0; i <= nLokdef; i++) {
				if(changedAddrIndex[i]) {
					changedAddrIndex[i]=false;
					addr_index=i;
					break;
				}
			}
			printf(" --- addr_index=%d\n", addr_index);
			//	if(changedAddrIndex[i]) { changedAddrIndex[i]=false;
			if(addr_index >= 0) {
				assert(addr_index >= 0);
				int a_speed=abs(lokdef[addr_index].currspeed);

				double f_speed=a_speed;
				if(f_speed < 5) {
					f_speed=0;
				} else {
				}

				// setPWM hängt beim RaspiPWM von den Funktionen ab
				bool func[MAX_NFUNC];
				for(int j=0; j < lokdef[addr_index].nFunc; j++) {
					func[j]=lokdef[addr_index].func[j].ison;
				}
				platine->setFunction(lokdef[addr_index].nFunc, func);

				platine->setDir(lokdef[addr_index].currdir < 0 ? 1 : 0 );
				platine->setPWM(f_speed);
				/*
				// int ia2=lokdef[addr_index].currdir < 0 ? 255 : 0; // 255 -> relais zieht an
				int ia2=0;
				printf("%d:lokdef[addr_index=%d].currspeed: %d dir: %d pwm1 val=>%d pwm2 %d (%f)\n",this->clientID,addr_index,lokdef[addr_index].currspeed,lokdef[addr_index].currdir,ia1,ia2,f_speed);
				// printf("lokdef[addr_index].currspeed=%d: ",lokdef[addr_index].currspeed);
				*/
				platine->commit();

#ifdef HAVE_ALSA
				sound.setSpeed(a_speed);
				if(lokdef[addr_index].func[1].ison && (cfg_funcSound[CFG_FUNC_SOUND_HORN] != "" )) {
					lokdef[addr_index].func[1].ison=false;
					PlayAsync horn(CFG_FUNC_SOUND_HORN);
					/*
					Sound horn;
					horn.init(SND_PCM_NONBLOCK);
					// horn.setBlocking(false);
					horn.playSingleSound(CFG_FUNC_SOUND_HORN);
					// horn.close(false);
					*/
				}
				if(lokdef[addr_index].func[2].ison && cfg_funcSound[CFG_FUNC_SOUND_ABFAHRT] != "") {
					lokdef[addr_index].func[2].ison=false;
					PlayAsync horn(CFG_FUNC_SOUND_ABFAHRT);
					/*
					Sound horn;
					horn.init(SND_PCM_NONBLOCK);
					// horn.setBlocking(false);
					horn.playSingleSound(CFG_FUNC_SOUND_ABFAHRT);
					// horn.close(false);
					*/
				}
#endif
			}
		} else
		if(srcp) { // erddcd/srcpd/dcc:
			// wegen X_MULTI gucken was wir geändert ham:
			for(int i=0; i <= nLokdef; i++) {
				if(changedAddrIndex[i]) {
					changedAddrIndex[i]=false;
					int addr_index=i;
					this->sendLoco(addr_index,emergencyStop);

				}
			}
		}
		msgNum++;
	}
	printf("%d:client exit\n",this->clientID);
}


void ClientThread::sendLoco(int addr_index, bool emergencyStop) {
	lokdef[addr_index].lastClientID = this->clientID;
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
						SRCPReplyPtr replyInit = srcp->sendLocoInit(lokdef[addr_index].addr, nFahrstufen, lokdef[addr_index].nFunc);
						if(replyInit->type != SRCPReply::OK) {
							fprintf(stderr,ANSI_RED "%d/%d: error init loco: (%s)\n" ANSI_DEFAULT, this->clientID, this->msgNum, replyInit->message);
							if(replyInit->code == 412) {
								fprintf(stderr,"%d/%d: loopback/ddl|number_gl, max addr < %d?\n", this->clientID, this->msgNum, lokdef[addr_index].addr);
								lokdef[addr_index].currspeed=-1;
							}
						} else {
							lokdef[addr_index].initDone=true;
							printf("try to read curr state...\n");
							if(!srcp->getInfo(lokdef[addr_index].addr,&dir,&dccSpeed,lokdef[addr_index].nFunc, func)) {
								fprintf(stderr,ANSI_RED "%d/%d: error getting state of loco: (%s)\n" ANSI_DEFAULT, this->clientID, this->msgNum, replyInit->message);
							}
						}
					}
					SRCPReplyPtr reply = srcp->sendLocoSpeed(lokdef[addr_index].addr, dir, nFahrstufen, dccSpeed, lokdef[addr_index].nFunc, func);

					if(reply->type != SRCPReply::OK) {
						fprintf(stderr,ANSI_RED "%d/%d: error sending speed: (%s)\n" ANSI_DEFAULT, this->clientID, this->msgNum, reply->message);
					}
}

/**
 * destruktor, schaut ob er der letzte clientThread war, wenn ja dann alle loks notstoppen
 */
ClientThread::~ClientThread()
{
	printf("%d:~ClientThread numClientd=%d\n",this->clientID, this->numClients);
	if(--this->numClients == 0) {
		// letzter client => alles notstop
		if(srcp) { // erddcd/srcpd/dcc:
			int addr_index=0;
			while(lokdef[addr_index].addr) {
				printf("last client [%d]=%d\n",addr_index,lokdef[addr_index].currspeed);
				if(lokdef[addr_index].currspeed != 0) {
					printf("emgstop [%d]=addr:%d\n",addr_index, lokdef[addr_index].addr);
					lokdef[addr_index].currspeed=0;
					this->sendLoco(addr_index, true);
					lokdef[addr_index].lastClientID=0;
				}
				addr_index++;
			}
		}
		else if(platine) {
			platine->fullstop();
		}
	} else {
		// nicht letzter client => alle loks die von mir gesteuert wurden notstop
		printf("lastClient, stopping my Locos\n");
		if(srcp) {
			int addr_index=0;
			while(lokdef[addr_index].addr) {
				if(lokdef[addr_index].lastClientID == this->clientID ) {
					printf("\tstop %d\n",addr_index);
					lokdef[addr_index].currspeed=0;
					this->sendLoco(addr_index, true);
					lokdef[addr_index].lastClientID=0;
				}
				addr_index++;
			}
		}
	}
	close(this->so);
}
