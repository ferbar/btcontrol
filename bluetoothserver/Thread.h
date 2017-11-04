#ifndef THREAD_H
#define THREAD_H 
#include <pthread.h>

class Thread {
public:
	Thread() : thread(0) {};
	virtual ~Thread() {};
	virtual void run()=0;
	void start();
	void self();
private:
	pthread_t thread;
	static void *startupThread(void *ptr);
};

#endif
