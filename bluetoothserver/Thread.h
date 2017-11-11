#ifndef THREAD_H
#define THREAD_H 
#include <pthread.h>

class Thread {
public:
	Thread() : thread(0),exited(false) {};
	virtual ~Thread() {};
	virtual void run()=0;
	void start();
	int self();
    void *cancel();
	bool isRunning();
	void testcancel();

private:
	pthread_t thread;
	static void *startupThread(void *ptr);
	bool exited;
};

#endif
