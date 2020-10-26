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
#include "CommThread.h"
#include <stdexcept>

#include <errno.h>

#ifdef INCL_BT
#include "BTUtils.h"
#endif

#include "utils.h"

static const char *TAG="CommThread";

void CommThread::sendMessage(const FBTCtlMessage &msg)
{
	if(!this->isConnected()) {
		throw std::runtime_error("client not connected");
	}
	std::string binMsg=msg.getBinaryMessage();
	int msgsize=binMsg.size();
	DEBUGF("/%d: sendMessage size: %zu+4 %d=%s", this->msgNum, binMsg.size(), msg.getType(), messageTypeName(msg.getType()).c_str());
	this->prepareMessage();
	this->write(&msgsize, 4);
	this->write(binMsg.data(), binMsg.size());
	this->flushMessage();
}

FBTCtlMessage CommThread::readMessage()
{
	if(!this->isConnected()) {
		throw std::runtime_error("client not connected");
	}
	int msgsize=0;
	int rc;
	/*
	struct timeval t,t0;
	gettimeofday(&t0, NULL);
	*/
	this->readSelect(); // auf daten warten, macht exception wenn innerhalb vom timeout nix kommt
	if((rc=this->read(&msgsize, 4)) != 4) {
		throw std::runtime_error(utils::format("error reading cmd: %d", rc));
	}
	/*
	gettimeofday(&t, NULL);
	int us=(t.tv_sec - t0.tv_sec) * 1000000 + t.tv_usec - t0.tv_usec;
	DEBUGF("select + read in %dµs",us);
	*/
	//  DEBUGF("%d:reading msg.size: %d bytes",this->clientID,msgsize);
	if(msgsize < 0 || msgsize > MAX_MESSAGE_SIZE) {
		throw std::runtime_error("invalid size msgsize 2big");
	}
	char buffer[msgsize];
	DEBUGF("readMessage: messagesize: %d", msgsize);
	this->readSelect();
	if((rc=this->read(buffer, msgsize)) != msgsize) {
		throw std::runtime_error(utils::format("error reading cmd.data: %d", rc));
	}
	// DEBUGF("%d:main loop - reader", this->clientID);
	InputReader in(buffer,msgsize);
	// DEBUGF("%d:parsing msg\n",this->clientID);
	FBTCtlMessage cmd;
	cmd.readMessage(in);
	return cmd;
}

/**
 * destruktor, schaut ob er der letzte clientThread war, wenn ja dann alle loks notstoppen
 */
CommThread::~CommThread()
{
	ERRORF(":~CommThread");
}
