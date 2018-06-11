#ifndef THREAD_H
#define THREAD_H 
#include <pthread.h>

class Thread {
public:
	Thread() : thread(0),exited(false),autodelete(false) {};
	virtual ~Thread() {};
	void start();
	int self();
    void *cancel();
	bool isRunning();
	void testcancel();
	// delete *this after run() exits
	void setAutodelete() {this->autodelete=true; } ;

private:
	virtual void run()=0;
	pthread_t thread;
	static void *startupThread(void *ptr);
	bool exited;
	bool autodelete;
};

class Mutex {
public:
	Mutex();
	~Mutex();
	void lock();
	void unlock();
	/// @return true if lock is successful
	bool tryLock();
private:
	pthread_mutex_t m;
};

class Lock {
public:
	explicit Lock(Mutex &m) : m(m) { m.lock(); } ;
	~Lock() { m.unlock(); } ;
private:
	Mutex &m;
};

#endif
