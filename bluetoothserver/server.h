#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <map>
#include <string>

class BTServer {
public:
	BTServer(int channel = 30);
	virtual ~BTServer();
	virtual int accept();
	static std::string getRemoteAddr(int so);
protected:
	int bt_so;
};

class Server : public BTServer {
public:
	Server();
	virtual ~Server() {};
	virtual int accept();
	void run();

private:
	int tcp_so;
	// für IDs
	int clientID_counter;
	// mapping clientID => pthreadID
	std::map<int,pthread_t> clients;
};

struct startupdata_t {
	int clientID;
	int so;
};

#endif
