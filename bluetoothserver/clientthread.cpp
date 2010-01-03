#include <stdio.h>
#include <stdlib.h>
#include "clientthread.h"
#include "lokdef.h"
#include "srcp.h"
#include "../usb k8055/k8055.h"
#include "utils.h"
#include "server.h"

extern int protocolHash;
extern K8055 *platine;
extern SRCP *srcp;


#define sendToPhone(text) \
		if(strlen(text) != write(startupdata->so,text,strlen(text))) { \
			printf("%d:error writing message (%s)\n",startupdata->clientID, strerror(errno)); \
			break; \
		} else { \
			printf("%d: ->%s",startupdata->clientID,text); \
		}

#define MAXPATHLEN 255

void ClientThread::sendMessage(const FBTCtlMessage &msg)
{
	std::string binMsg=msg.getBinaryMessage();
	int msgsize=binMsg.size();
	write(this->so, &msgsize, 4);
	write(this->so, binMsg.data(), binMsg.size());
	printf("messagesize: %d+4\n",binMsg.size());
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

void ClientThread::sendClientUpdate()
{
	std::string clientAddr = BTServer::getRemoteAddr(this->so);
	for(int i=0; i < 10; i++) {
		printf("."); fflush(stdout);
		sleep(1);
	}
	printf("BT send update: to %s\n", clientAddr.c_str());
	std::string cmd;
	cmd+="./ussp-push " + clientAddr + "@ btcontroll.jar btcontroll.jar";
	system(cmd.c_str());
}

/**
 * für jedes handy ein eigener thread...
 */
void ClientThread::run()
{
	// startupdata_t *startupdata=(startupdata_t *)data;

	try {
	printf("%d:socket accepted sending welcome msg\n",this->clientID);
	FBTCtlMessage heloReply(messageTypeID("HELO"));
	heloReply["name"]="my bt server";
	heloReply["version"]="0.9";
	heloReply["protohash"]=protocolHash;
	// heloReply.dump();
	sendMessage(heloReply);

	int nLokdef=0;
	while(lokdef[nLokdef].addr) nLokdef++;
	lastStatus_t lastStatus[nLokdef+1];
	for(int i=0; i <= nLokdef; i++) { lastStatus[i].func=-1; }
	bool changedAddrIndex[nLokdef+1];
	for(int i=0; i <= nLokdef; i++) { changedAddrIndex[i] = false; }


	int x=0;
	// int speed=0;
	// int addr=3;
	while(1) {
		// addr_index sollte immer gültig sin - sonst gibts an segv ....
	
		bool emergencyStop=false;

		int msgsize;
		int rc;
		if((rc=read(this->so, &msgsize, 4)) != 4) {
			throw std::string("error reading cmd: ") += rc; // + ")";
		}
		printf("%d:reading msg.size: %d bytes\n",this->clientID,msgsize);
		if(msgsize < 0 || msgsize > 10000) {
			throw std::string("invalid size 2big");
		}
		char buffer[msgsize];
		if((rc=read(this->so, buffer, msgsize)) != msgsize) {
			throw std::string("error reading cmd.data: ") += rc; // + ")";
		}
		InputReader in(buffer,msgsize);
		printf("%d:parsing msg\n",this->clientID);
		FBTCtlMessage cmd;
		cmd.readMessage(in);
		// cmd.dump();
		int nr=0; // für die conrad platine
		/*
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
					throw "invalid dir";
				}
				if(lokdef[addr_index].currspeed != 0) {
					throw "speed != 0";
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
				cmd.dump();
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
					if(value)
						lokdef[addr_index].func[funcNr].ison = true;
					else
						lokdef[addr_index].func[funcNr].ison = false;
				} else {
					printf("%d:invalid funcNr out of bounds(%d)\n",this->clientID,funcNr);
				}
				sendStatusReply(lastStatus);
				changedAddrIndex[addr_index]=true;

/*
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
				printf("funclist for addr %d\n",addr);
				// char buffer[32];
				for(int i=0; i < lokdef[addr_index].nFunc; i++) {
					// snprintf(buffer,sizeof(buffer)," %d;%d;%s\n",j+1,lokdef[addr_index].func[j].ison,lokdef[addr_index].func[j].name);
					reply["info"][i]["name"]=lokdef[addr_index].func[i].name;
					reply["info"][i]["value"]=lokdef[addr_index].func[i].ison;
					reply["info"][i]["imgname"]="";
				}
				sendMessage(reply);
			} else if(cmd.isType("POWER")) {
				int value=cmd["value"].getIntVal();
				if(srcp) {
					if(value) srcp->pwrOn();
					else srcp->pwrOff();
				}
				FBTCtlMessage reply(messageTypeID("POWER_REPLY"));
				reply["value"]=value;
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
					reply["value"]=0;
				}
				sendMessage(reply);
			} else if(cmd.isType("GETIMAGE")) {
				std::string imageName=cmd["imgname"].getStringVal();
				FBTCtlMessage reply(messageTypeID("GETIMAGE_REPLY"));
				reply["img"]=readFile("img/"+imageName);
				sendMessage(reply);
			} else if(cmd.isType("HELO_ERR")) {
				printf("client proto error\n");
				int protohash=cmd["protohash"].getIntVal();
				int doupdate=cmd["doupdate"].getIntVal();
				printf("hash=%d (me:%d), doupdate=%d\n",protohash,protocolHash,doupdate);
				this->sendClientUpdate();
			} else {
				printf("%d:----------------- invalid/unimplemented command (%d,%s)------------------------\n",
					this->clientID,cmd.getType(),messageTypeName(cmd.getType()).c_str());
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
			int changedAddr=1;
			if(changedAddr) {
				int addr_index=getAddrIndex(changedAddr);
				unsigned int a_speed=abs(lokdef[addr_index].currspeed);
				double f_speed=a_speed;

				int ia1=255-(int)f_speed;
				printf("%d:lokdef[addr_index].currspeed: %d (%f)\n",this->clientID,ia1,f_speed);
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
			}
		} else if(srcp) { // erddcd/srcpd/dcc:
			// wegen X_MULTI gucken was wir geändert ham:
			for(int i=0; i <= nLokdef; i++) {
				if(changedAddrIndex[i]) {
					changedAddrIndex[i]=false;
					int addr_index=i;
					int dir= lokdef[addr_index].currdir < 0 ? 0 : 1;
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
						srcp->sendLocoInit(lokdef[addr_index].addr, nFahrstufen, lokdef[addr_index].nFunc);
						lokdef[addr_index].initDone=true;
					}
					srcp->sendLocoSpeed(lokdef[addr_index].addr, dir, nFahrstufen, dccSpeed, lokdef[addr_index].nFunc, func);
				}
			}
		}
	}
	printf("%d:client exit\n",this->clientID);
	} catch(const char *e) {
		printf("%d:exception %s\n",this->clientID,e);
	} catch(std::string &s) {
		printf("%d:exception %s\n",this->clientID,s.c_str());
	}
	close(this->so);
}
