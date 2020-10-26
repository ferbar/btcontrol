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

class Condition;

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
friend class Condition;
};

class Lock {
public:
	explicit Lock(Mutex &m) : m(m) { m.lock(); } ;
	~Lock() { m.unlock(); } ;
private:
	Mutex &m;
};

class Condition {
public:
	Condition();
	void wait();
	/// true=signal, false=timeout
	bool timeoutWait(int timeout);
	void signal();
private:
	Mutex mutex;
	pthread_cond_t cond; // ESP32 bug - geht nicht: = PTHREAD_COND_INITIALIZER;
};

/**
 * threadspecific wrapper klasse
 * muss global sein!
 */
class ThreadSpecific {
public:
	ThreadSpecific();
	virtual ~ThreadSpecific();
	virtual void *get();
	virtual void set(void *ptr);
// TODO: wird beim beenden vom thread aufgerufen
	virtual void del();
private:
	pthread_key_t key;
//	pthread_once_t key_once;
};
#endif
