#include "Thread.h"
#include "stdio.h"
#include <stdexcept>


void *Thread::startupThread(void *ptr) {
	printf("Thread::startupThread()\n");
	Thread *t=(Thread *) ptr;
	t->run();
	printf("Thread::startupThread() done\n");
	t->exited=true;
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
