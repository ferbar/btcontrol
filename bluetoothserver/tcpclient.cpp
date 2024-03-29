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
#include <stdexcept>

// für setsockopt
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

// für getpeername
#include <arpa/inet.h>

#include <errno.h>

#include "utils.h"
#include "server.h"
#include "tcpclient.h"
#include "BTUtils.h"

#define TAG "TCPClient"

TCPClient::TCPClient(int so) : so(so) {
	NOTICEF("TCPClient::TCPClient remote addr: %s", this->getRemoteAddr().c_str());
}

void TCPClient::prepareMessage()
{
	int flag = 1;
	setsockopt(this->so, IPPROTO_TCP, TCP_CORK, (char *)&flag, sizeof(flag) ); // prepare message, stopsel rein
}

void TCPClient::flushMessage()
{
	int flag=0;
	setsockopt(this->so, IPPROTO_TCP, TCP_CORK, (char *)&flag, sizeof(flag) ); // message fertig, senden
}

/**
 * wartet bis daten daherkommen, macht exception wenn keine innerhalb von timeout gekommen sind
 */
void TCPClient::readSelect(int timeout)
{
	struct timeval t;
	fd_set set;
	t.tv_sec=timeout; t.tv_usec=0;
	FD_ZERO(&set); FD_SET(this->so,&set);
	int rc;
	if((rc=select(this->so+1, &set, NULL, NULL, &t)) <= 0) {
		if(rc != 0) {
			ERRORF("ClientThread::readSelect error in select(%d)=%d %s", this->so, rc, strerror(errno));
			throw std::runtime_error("error select");
		}
		ERRORF("ClientThread::readSelect timeout in select(%d)=%d timeout=%ds", this->so, rc, timeout);
		throw std::runtime_error("timeout reading cmd");
	}
}


/**
 * destruktor
 */
TCPClient::~TCPClient()
{
	this->close();
}

void TCPClient::close() {
	::close(this->so);
	this->so=-1;
}

std::string TCPClient::getRemoteAddr() {
	struct sockaddr_in addr;
	socklen_t addrlen=sizeof(addr);
	if(getsockname(this->so, (sockaddr*) &addr, &addrlen) == 0) {
		if(addr.sin_family == AF_INET) {
			struct sockaddr_in peeraddr;
			socklen_t peeraddrlen = sizeof(peeraddr);
			getpeername(this->so, (sockaddr*) &peeraddr, &peeraddrlen);
			std::string ret;
			ret.resize(INET_ADDRSTRLEN+1);
			inet_ntop(AF_INET, &(peeraddr.sin_addr), (char *) ret.c_str(), INET_ADDRSTRLEN);
			return ret;
#ifdef INCL_BT
		} else if(addr.sin_family == AF_BLUETOOTH) {
			return BTUtils::getRemoteAddr(this->so);
#endif
		} else {
			ERRORF("TCPClient::getRemoteAddr unknown family:%d", addr.sin_family);
			return "unknown";
		}
	}
	ERRORF("getsockname failed");
	return "";
}

ssize_t TCPClient::read(void *buf, size_t count) {
	try {
		return ::myRead(this->so, buf, count);
	} catch(...) {
		this->close();
		throw;
	}
}

ssize_t TCPClient::write(const void *buf, size_t count) {
	try {
		return ::write(this->so, buf, count);
	} catch(...) {
		this->close();
		throw;
	}
}
