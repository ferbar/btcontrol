// sonst geht clock_gettime am ESP32 nicht
#define _POSIX_TIMERS
#include <time.h>

#include <stdio.h>
#include <stdexcept>
#include <sys/errno.h>
#include <stdlib.h>
#include <cxxabi.h>
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "Thread.h"

#if defined ESP32 && defined DEBUG_MUTEX
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

#define TAG "Thread"

void *Thread::startupThread(void *ptr) {
	Thread *t=(Thread *) ptr;
	// %lu => linux ist pthread_ unsigned long
	NOTICEF("Thread::[%x] @%p startupThread name=%s ========================================", t->getMyId(), t, t->which());
	t->exited=false;
	try {
		t->run();

// !!!!! looks as if ESP32 idf 4.4 can only handle 2 different exceptions, 202203: all exceptions should be std::exception
#ifndef ESP32
	} catch(const char *e) {
		ERRORF("?: exception %s - client thread killed", e);
	} catch(const std::RuntimeExceptionWithBacktrace &e) {
		ERRORF("?: Runtime Exception with Backtrace %s - client thread killed", e.what());
	} catch(const std::runtime_error &e) {
		ERRORF("?: Runtime Exception %s - client thread killed", e.what());
#endif
	} catch(const std::exception &e) {
		ERRORF("?: exception %s - client thread killed", e.what());
#ifndef ESP32
	} catch (abi::__forced_unwind&) { // http://gcc.gnu.org/bugzilla/show_bug.cgi?id=28145
		ERRORF("Thread::[%d] done: forced unwind exception - client thread killed", t->getMyId());
		// copy &paste:
		// printf("%d:client exit\n",startupData->clientID);
		// pthread_cleanup_pop(true);
		t->exited=true;
		if(t->autodelete) {
			delete t;
		}
		throw; // rethrow exeption bis zum pthread_create, dort isses dann aus
#endif
	} catch(...) {
		ERRORF("?: unknown exception caught!"); // crash otherwise
	}

	NOTICEF("[%x] %s done ====================================", t->getMyId(), t->which() );
	t->exited=true;
	if(t->autodelete) {
		DEBUGF("deleting object %p", t);
		delete t;
	}
	return NULL;
}

void Thread::start() {
	if(this->isRunning()) {
		throw std::runtime_error("Thread::start() => thread already running!");
	}
	pthread_attr_t attr;
	pthread_attr_init(&attr);
#ifdef ESP32
	DEBUGF("set stack size to 10000");
	pthread_attr_setstacksize(&attr,10000);
	this->cancelstate=0;
#endif
	int s = pthread_create(&this->thread, &attr, &Thread::startupThread, (void *) this);
	if (s != 0) {
		perror("pthread_create");
		throw std::runtime_error("error pthread_create");
	}
	pthread_attr_destroy(&attr);
}

void Thread::cancel(bool join) {
	NOTICEF("Thread::cancel(join=%d) thread=%0lx", join, this->thread);
	if(this->thread) {
#ifdef ESP32
    // ESP lib kennt kein pthread_cancel
		this->cancelstate=1;
#else
		int s = pthread_cancel(this->thread);
		if (s != 0) {
			perror("pthread_cancel");
			throw std::runtime_error("error pthread_cancel");
		}
#endif
		if(join) {
			void *ret=NULL;
			// %lu => linux ist pthread_ unsigned long
			DEBUGF("Thread::[%0lx] cancel() waiting to exit", this->thread);
			int rc = pthread_join(this->thread, &ret);
			if(ret == PTHREAD_CANCELED) {
				NOTICEF("Thread::[%0lx] cancel() join=true thread was canceled", this->thread);
			} else if(ret != NULL) { // if this is malloced value we create a memory leak
				ERRORF("Thread::[%0lx] cancel() join=true, ret=%p", this->thread, ret);
				abort();
			}			
			if(rc != 0) {
				throw std::runtime_error("error pthread_join");
			}
		}
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
		DEBUGF("Thread::[%0lx] quitting thread with an exception", this->getMyId());
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
#ifdef DEBUG_MUTEX
	this->lockedby=0;
#endif
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
#ifdef DEBUG_MUTEX
	if(this->lockedby) {
		DEBUGF("mutex locked by %0x", this->lockedby);
	}
#endif
	if(pthread_mutex_lock(&this->m) != 0 ) {
		throw std::runtime_error("error Mutex::lock()");
	}
#ifdef DEBUG_MUTEX
#ifdef ESP32
	// esp32 crashes if pthread_self() is called in main loop!!! => getName returns "loop"
	if(STREQ(pcTaskGetName(NULL), "pthread"))
#endif
		this->lockedby=pthread_self();
#ifdef ESP32
	else
		this->lockedby=-1;
#endif
#endif
}

void Mutex::unlock() {
	if(pthread_mutex_unlock(&this->m) != 0 ) {
		throw std::runtime_error("error Mutex::unlock()");
	}
#ifdef DEBUG_MUTEX
	this->lockedby=0;
#endif
}

bool Mutex::tryLock() {
#ifdef DEBUG_MUTEX
	if(this->lockedby) {
		DEBUGF("mutex locked by %0x", this->lockedby);
	}
#endif
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

