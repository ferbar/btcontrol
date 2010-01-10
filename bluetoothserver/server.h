#include <pthread.h>
#include <map>
#include <string>

#define STREQ(s1,s2) (strcmp(s1,s2)==0)

class BTServer {
public:
	BTServer(int channel = 30);
	virtual ~BTServer();
	virtual int accept();
	static std::string getRemoteAddr(int so);
protected:
	int so;
};

class Server : public BTServer {
public:
	Server();
	virtual ~Server() {};
	virtual int accept();
	void run();

private:
	int tcp_so;
	int clientID_counter;
	// mapping clientID => pthreadID
	std::map<int,pthread_t> clients;
};

struct startupdata_t {
	int clientID;
	int so;
};

