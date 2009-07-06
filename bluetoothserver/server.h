#include <pthread.h>
#include <map>

class BTServer{
public:
	BTServer(int channel = 30);
	virtual ~BTServer();
	virtual int accept();
protected:
	int so;
};

class Server : public BTServer {
public:
	Server();
	virtual ~Server() {};
	virtual int accept();
	std::map<int,pthread_t> clients;
	int clientID_counter;

private:
	int tcp_so;
};

struct startupdata_t {
	int clientID;
	int so;
};

