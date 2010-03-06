/*
 *  This file is part of btcontroll
 *  btcontroll is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  btcontroll is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with btcontroll.  If not, see <http://www.gnu.org/licenses/>.
 *
 * TCP - server
 */
#include <sys/select.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>

#include "server.h"
#include "clientthread.h"

Server::Server()
: clientID_counter(1)
{

	// init socket
	int on=1;
	int port=3030;
	int rc;

	struct sockaddr_in address;
	bzero((char *)&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	this->tcp_so = socket(AF_INET, SOCK_STREAM, 0);
	if(this->tcp_so < 0) {
		perror("Error in socket ");
		exit(1);
	}
	setsockopt(this->tcp_so, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

	// potentialy not threadsafe !
	// struct hostent *hp=gethostbyname("localhost");
	struct hostent *hp=gethostbyname("0.0.0.0");
	if(! hp) {
		perror("invalid IP");
		exit(1);
	}

	memcpy((char *)&address.sin_addr,(char *)hp->h_addr,(size_t)hp->h_length);

	rc = bind(this->tcp_so, (struct sockaddr *)&address, sizeof(address));

	if (rc!=0)
	{
		perror("bind error");
		exit(1);
	}
	rc = ::listen(this->tcp_so, 20);
	if (rc!=0)
	{
		perror("Error in Listen");
		exit(1);
	}
}
 
int Server::accept()
{
	// assert(this->bt_so > 0); - keine BT hardware -> this->bt_so < 0
	assert(this->tcp_so > 0); // FD_SET mit -1 macht was hin

	fd_set fd;
	FD_ZERO(&fd);
	if(this->tcp_so > 0) {
		FD_SET(this->bt_so,&fd);
	}
	FD_SET(this->tcp_so,&fd);
	int rc=select(FD_SETSIZE, &fd, NULL, NULL, NULL);
	printf("-----select rc=%d\n",rc);
	if(this->bt_so > 0 && FD_ISSET(this->bt_so,&fd)) {
		printf("bt connection\n");
		return BTServer::accept();
	} else if(FD_ISSET(this->tcp_so,&fd)) {
		printf("tcp connection\n");

		struct sockaddr_in addr2;
		socklen_t siz;
		siz = (socklen_t)sizeof(addr2);
		int csock = ::accept(this->tcp_so, (struct sockaddr *)&addr2, &siz);
		if(csock < 0) {
			perror("so->accept error:");
			exit(1);
		}
		printf("socket: %d\n",csock);
		return csock;
	}
	/*
	   struct linger ling;
	   ling.l_onoff=1;
	   ling.l_linger=10;
	   setsockopt(csock, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling)); */

	perror("select/accept error");
	return -1;
}

/**
 * unregister handler
 */
static void unregisterPhoneClient(void *data)
{
	startupdata_t *startupData=(startupdata_t *)data;
	free(startupData);
}

/**
 * für jedes handy ein eigener thread...
 */
static void *phoneClient(void *data)
{
	startupdata_t *startupData=(startupdata_t *)data;
	printf("%d:new client\n",startupData->clientID);
	pthread_cleanup_push(unregisterPhoneClient,data);

	try {
		ClientThread client(startupData->clientID, startupData->so);
		client.run();
	} catch(const char *e) {
		printf("%d:exception %s\n",startupData->clientID,e);
	} catch(std::string &s) {
		printf("%d:exception %s\n",startupData->clientID,s.c_str());
	}
	printf("%d:client exit\n",startupData->clientID);

	pthread_cleanup_pop(true);
	return NULL;
}

void Server::run()
{
	while(1) {
		int nsk = this->accept();
	// client thread vorbereiten + starten
		startupdata_t *startupData=(startupdata_t*) calloc(sizeof(startupdata_t),1);
		startupData->clientID=this->clientID_counter++;
		startupData->so=nsk;
		pthread_t &newThread=this->clients[startupData->clientID];
		bzero(&newThread,sizeof(newThread));
		pthread_create(&newThread, NULL, phoneClient, (void *)startupData);
	}
}
