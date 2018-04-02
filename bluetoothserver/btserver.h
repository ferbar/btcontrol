#ifndef BTSERVER_H
#define BTSERVER_H

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

#endif
