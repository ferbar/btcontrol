// sonst geht clock_gettime am ESP32 nicht
#define _POSIX_TIMERS
#include <time.h>

#include <stdio.h>
#include <stdexcept>
#include <sys/errno.h>
#include <stdlib.h>
#include <cxxabi.h>
#include <string.h>
#include "utils.h"
#include "Thread.h"

static const char *TAG="THREAD";

void *Thread::startupThread(void *ptr) {
	NOTICEF("Thread::startupThread() ========================================");
	Thread *t=(Thread *) ptr;
	t->exited=false;
	try {
		t->run();
	} catch(const char *e) {
		ERRORF(ANSI_RED "?: exception %s - client thread killed\n" ANSI_DEFAULT, e);
	} catch(std::RuntimeExceptionWithBacktrace &e) {
		ERRORF(ANSI_RED "?: Runtime Exception %s - client thread killed\n" ANSI_DEFAULT, e.what());
	} catch(std::exception &e) {
		ERRORF(ANSI_RED "?: exception %s - client thread killed\n" ANSI_DEFAULT, e.what());
	} catch (abi::__forced_unwind&) { // http://gcc.gnu.org/bugzilla/show_bug.cgi?id=28145
		ERRORF(ANSI_RED "?: forced unwind exception - client thread killed\n" ANSI_DEFAULT);
		// copy &paste:
		// printf("%d:client exit\n",startupData->clientID);
		// pthread_cleanup_pop(true);
		throw; // rethrow exeption bis zum pthread_create, dort isses dann aus
	}

	NOTICEF("Thread::startupThread() done ====================================");
	t->exited=true;
	if(t->autodelete) {
		delete t;
	}
	return NULL;
}

void Thread::start() {
	pthread_attr_t attr;
	pthread_attr_init(&attr);
#ifdef ESP32
	DEBUGF("set stack size to 10000");
	pthread_attr_setstacksize(&attr,10000);
	this->cancelstate=0;
#endif
	int s = pthread_create(&this->thread, &attr, &Thread::startupThread, (void *) this);
	if (s != 0)
		perror("pthread_create");
	pthread_attr_destroy(&attr);
}

void Thread::cancel() {
	NOTICEF("Thread::cancel()\n");
	if(this->thread) {
#ifdef ESP32
    // ESP lib kennt kein pthread_cancel
		this->cancelstate=1;
#else
		int s = pthread_cancel(this->thread);
		if (s != 0)
			perror("pthread_cancel");
			throw std::runtime_error("error pthread_cancel");
		int rc = pthread_join(this->thread, &ret);
		if(rc != 0) {
			// FIXME: memory leak wenn ret malloced ist
			throw std::runtime_error("error pthread_join");
		}
#endif
		this->thread=0;
	} else {
		// already killed/never started
		throw std::runtime_error("thread not started");
	}
}

bool Thread::isRunning() {
	return this->thread != 0 && ! this->exited;
}

void Thread::testcancel() {
#ifdef ESP32
	if(this->cancelstate) {
		throw std::runtime_error("thread canceled from testcancel()"); // pthread_testcancel macht auch nur exception
	}
#else
	pthread_testcancel();
#endif
}

int Thread::self() {
	return pthread_self();
}

Mutex::Mutex() {
	if(pthread_mutex_init(&this->m, NULL) != 0) {
		throw std::runtime_error("error creating mutex");
	}
}

Mutex::~Mutex() {
	if(! this->tryLock()) {
		ERRORF("error destroying mutex - mutex is locked");
		abort();
	}
	this->unlock();
	if(pthread_mutex_destroy(&this->m)) {
		ERRORF("error destroying mutex");
		abort();
	}
}

void Mutex::lock() {
	if(pthread_mutex_lock(&this->m) != 0 ) {
		throw std::runtime_error("error Mutex::lock()");
	}
}

void Mutex::unlock() {
	if(pthread_mutex_unlock(&this->m) != 0 ) {
		throw std::runtime_error("error Mutex::unlock()");
	}
}

bool Mutex::tryLock() {
	// 0 on success
	int ret = pthread_mutex_trylock(&this->m);
	if(ret == 0) return true;
	if(ret == EBUSY) return false;
	throw std::runtime_error("error Mutex::trylock()");
}

void ThreadSpecific_Descructor(void *ptr) {
}

ThreadSpecific::ThreadSpecific() {
	pthread_key_create(&this->key,ThreadSpecific_Descructor);
}

ThreadSpecific::~ThreadSpecific() {
	pthread_key_delete(this->key);
}

void *ThreadSpecific::get() {
	return pthread_getspecific(this->key);
}

void ThreadSpecific::set(void *ptr) {
	pthread_setspecific(this->key, ptr);
}

void ThreadSpecific::del() {
	pthread_setspecific(this->key, NULL);
}

Condition::Condition() {
	pthread_cond_init(&this->cond, NULL);
}

void Condition::wait() {
	Lock lock(this->mutex);
}

/**
 * @param timeout
 *     timeout in seconds
 * rc
 * true=signal, false=timeout
 */
bool Condition::timeoutWait(int timeout) {
	Lock lock(this->mutex);

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += timeout;

	int rc = pthread_cond_timedwait(&this->cond, &this->mutex.m, &ts);
	if(rc == 0) {
		return true;
	}
	if(rc == ETIMEDOUT) {
		return false;
	}
	throw std::runtime_error(utils::format("Condition::timeoutWait() error %s", strerror(rc)));
}

void Condition::signal() {
	Lock lock(this->mutex);
	const int signal_rv = pthread_cond_signal(&(this->cond));
    if (signal_rv) {
		throw std::runtime_error(utils::format("Condition::signal() error %s", strerror(signal_rv)));
	}
}

