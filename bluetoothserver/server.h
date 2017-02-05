#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <map>
#include <string>
#include "fbtctl_message.h"

class BTServer {
public:
	BTServer(int channel = 30);
	virtual ~BTServer();
	virtual int accept();
	static std::string getRemoteAddr(int so);
	static void pushUpdate(int so);
	static void BTPush(std::string addr);
	static void BTScan(FBTCtlMessage &reply);
protected:
	int bt_so;
};

class Server
#ifdef INCL_BT
 : public BTServer
#endif
 {
public:
	Server(int port);
	virtual ~Server() {};
	virtual int accept();
	void run();
	static void setExit();
	static void waitExit();
	static void clientDone(int clientID);
private:
	int tcp_so;
	// für IDs
	int clientID_counter;
	// mapping clientID => pthreadID
	static bool isrunning;
	static bool exit;
	static std::map<int,pthread_t> clients;
};

struct startupdata_t {
	int clientID;
	int so;
};

#endif
