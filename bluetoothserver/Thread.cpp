#include "Thread.h"
#include "stdio.h"
#include <stdexcept>
#include <sys/errno.h>


void *Thread::startupThread(void *ptr) {
	printf("Thread::startupThread()\n");
	Thread *t=(Thread *) ptr;
	t->run();
	printf("Thread::startupThread() done\n");
	t->exited=true;
	if(t->autodelete) {
		delete t;
	}
	return NULL;
}

void Thread::start() {
	int s = pthread_create(&this->thread, NULL, &Thread::startupThread, (void *) this);
	if (s != 0)
		perror("pthread_create");
}

void *Thread::cancel() {
	printf("Thread::cancel()\n");
	void *ret;
	if(this->thread) {
		int s = pthread_cancel(this->thread);
		if (s != 0)
			perror("pthread_cancel");
		int rc = pthread_join(this->thread, &ret);
		if(rc != 0) {
			// FIXME: memory leak wenn ret malloced ist
			throw std::runtime_error("error pthread_join");
		}
		this->thread=0;
	} else {
		// already killed/never started
		throw std::runtime_error("thread not started");
	}
	return ret;
}

bool Thread::isRunning() {
	return this->thread != 0 && ! this->exited;
}

void Thread::testcancel() {
	pthread_testcancel();
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
		throw std::runtime_error("error destroying mutex - mutex is locked");
	}
	this->unlock();
	if(pthread_mutex_destroy(&this->m)) {
		throw std::runtime_error("error destroying mutex");
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

