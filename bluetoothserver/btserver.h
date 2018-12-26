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
protected:
	int bt_so;
};

#endif
