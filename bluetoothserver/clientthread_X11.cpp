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
 * client - thread - part (für jeden client ein thread) - X11 ctl version
 *
 * damit kann man cursor links + rechts [acc | break] an das Programm mit fokus senden
 */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include "clientthread_X11.h"
#include "lokdef.h"
#include <stdexcept>
#include "config.h"
#ifdef INCL_BT
#include "BTUtils.h"
#endif

#include "utils.h"

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>


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
void ClientThreadX11::run()
{
	// startupdata_t *startupdata=(startupdata_t *)data;

	utils::setThreadClientID(this->clientID);
	NOTICEF("%d:socket accepted sending welcome msg",this->clientID);
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


	NOTICEF("%d:hello done, enter main loop",this->clientID);
	// int speed=0;
	// int addr=3;
	while(1) {
		// addr_index sollte immer gültig sin - sonst gibts an segv ....
	
		// bool emergencyStop=false;


		FBTCtlMessage cmd=this->readMessage();

		if(cfg_debug) {
			DEBUGF("/%d: msg", this->msgNum);
			cmd.dump();
		} else {
			DEBUGF("/%d: msg %d=%s", this->msgNum, cmd.getType(), messageTypeName(cmd.getType()).c_str());
		}
		/*
		int nr=0; // für die conrad platine
		int size;
		char buffer[256];
		if((size = read(startupdata->so, buffer, sizeof(buffer))) <= 0) {
			DEBUGF("%d:error reading message size=%d \"%.*s\"\n",startupdata->clientID,size,size >= 0 ? buffer : NULL);
			break;
		}
		buffer[size]='\0';
		DEBUGF("%.*s",size,buffer);
		char cmd[sizeof(buffer)]="";
		char param1[sizeof(buffer)]="";
		char param2[sizeof(buffer)]="";
		sscanf(buffer,"%d %s %s %s",&nr,&cmd, &param1, &param2);
		DEBUGF("%d:<- %s p1=%s p2=%s\n",startupdata->clientID,cmd,param1,param2);
		*/
		if(true) {
			if(cmd.isType("PING")) {
				sendStatusReply(lastStatus);
			} else if(cmd.isType("ACC")) {
				int addr=cmd["addr"].getIntVal();
				int addr_index=getAddrIndex(addr);
				lokdef[addr_index].currspeed+=SPEED_STEP;
				if(lokdef[addr_index].currspeed > 255)
					lokdef[addr_index].currspeed=255;
				sendStatusReply(lastStatus);
				changedAddrIndex[addr_index]=true;

				Display *dpy = XOpenDisplay( NULL );
				XTestFakeKeyEvent( dpy, XKeysymToKeycode( dpy, XK_Right ), True, CurrentTime );
				XTestFakeKeyEvent( dpy, XKeysymToKeycode( dpy, XK_Right ), False, CurrentTime );
				XCloseDisplay( dpy );
			} else if(cmd.isType("BREAK")) {
				int addr=cmd["addr"].getIntVal();
				int addr_index=getAddrIndex(addr);
				lokdef[addr_index].currspeed-=SPEED_STEP;
				if(lokdef[addr_index].currspeed < 0)
					lokdef[addr_index].currspeed=0;
				sendStatusReply(lastStatus);
				changedAddrIndex[addr_index]=true;

				Display *dpy = XOpenDisplay( NULL );
				XTestFakeKeyEvent( dpy, XKeysymToKeycode( dpy, XK_Left ), True, CurrentTime );
				XTestFakeKeyEvent( dpy, XKeysymToKeycode( dpy, XK_Left ), False, CurrentTime );
				XCloseDisplay( dpy );
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

				Display *dpy = XOpenDisplay( NULL );
				XTestFakeKeyEvent( dpy, XKeysymToKeycode( dpy, XK_Home ), True, CurrentTime );
				XTestFakeKeyEvent( dpy, XKeysymToKeycode( dpy, XK_Home ), False, CurrentTime );
				XCloseDisplay( dpy );

			} else if(cmd.isType("ACC_MULTI")) {
				int n=cmd["list"].getArraySize();
				int addr=cmd["list"][0]["addr"].getIntVal();
				int addr_index=getAddrIndex(addr);
				lokdef[addr_index].currspeed+=SPEED_STEP;
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
				lokdef[addr_index].currspeed-=SPEED_STEP;
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
					// if(cfg_debug)
					DEBUGF("/%d:set funcNr[%d]=%d", this->msgNum, funcNr, value);
					if(value)
						lokdef[addr_index].func[funcNr].ison = true;
					else
						lokdef[addr_index].func[funcNr].ison = false;
				} else {
					ERRORF("/%d:invalid funcNr out of bounds(%d)", this->msgNum, funcNr);
				}
				sendStatusReply(lastStatus);
				changedAddrIndex[addr_index]=true;

/*
			} else if(STREQ(cmd,"select")) { // ret = lokname
				int new_addr=atoi(param1);
				DEBUGF("%d:neue lok addr:%d\n",startupdata->clientID,new_addr);
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
				DEBUGF("/%d: funclist for addr %d", this->msgNum, addr);
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
				FBTCtlMessage reply(messageTypeID("POWER_REPLY"));
				reply["value"]=value;
				sendMessage(reply);
			} else if(cmd.isType("POM")) {
				/*
				int addr=cmd["addr"].getIntVal();
				int addr_index=getAddrIndex(addr);
				int cv=cmd["cv"].getIntVal();
				int value=cmd["value"].getIntVal();
				*/
				FBTCtlMessage reply(messageTypeID("POM_REPLY"));
				sendMessage(reply);
			} else if(cmd.isType("GETIMAGE")) {
				std::string imageName=cmd["imgname"].getStringVal();
				FBTCtlMessage reply(messageTypeID("GETIMAGE_REPLY"));
				if(imageName == "") {
					ERRORF("requesting null file");
					reply["img"]="";
				} else {
					reply["img"]=readFile("img/"+imageName);
				}
				sendMessage(reply);
			} else if(cmd.isType("HELO_ERR")) {
				ERRORF("/%d: client proto error", this->msgNum);
				int protohash=cmd["protohash"].getIntVal();
				int doupdate=cmd["doupdate"].getIntVal();
				ERRORF("hash=%d (me:%d), doupdate=%d",protohash,messageLayouts.protocolHash,doupdate);
				this->sendClientUpdate(NOT_SET);
#ifdef INCL_BT
			} else if(cmd.isType("BTSCAN")) { // liste mit eingetragenen loks abrufen, format: <name>;<adresse>;...\n
				FBTCtlMessage reply(messageTypeID("BTSCAN_REPLY"));
				BTUtils::BTScan(this->client->getRemoteAddr().c_str(), reply);
				// reply.dump();
				sendMessage(reply);
			} else if(cmd.isType("BTPUSH")) { // sendUpdate
				DEBUGF("BTPUSH ---------------------------------------------------");
				FBTCtlMessage reply(messageTypeID("BTPUSH_REPLY"));
				std::string addr=cmd["addr"].getStringVal();
				// TODO: ussppush oder gammu push 
				// int type=cmd["type"].getIntVal();
				this->sendClientUpdate(addr);

				// reply.dump();
				reply["rc"]=1;
				sendMessage(reply);
#endif
			} else {
				ERRORF("/%d:----------------- invalid/unimplemented command (%d,%s)------------------------",
					this->msgNum, cmd.getType(), messageTypeName(cmd.getType()).c_str());
			/*
			if(memcmp(cmd,"invalid_key",10)==0) {
				DEBUGF("%d:invalid key ! param1: %s\n",startupdata->clientID,param1);
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
					DEBUGF("notaus\n");
					lokdef[addr_index].currspeed=0;
					emergencyStop=true;
				}
				*/
			}
		} else {
			ERRORF(":no command?????");
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

	}
	NOTICEF("%d:client exit",this->clientID);
}

/**
 * destruktor
 */
ClientThreadX11::~ClientThreadX11()
{
}
