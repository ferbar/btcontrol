#include "Thread.h"
#include "stdio.h"


void *Thread::startupThread(void *ptr) {
	printf("Thread::startupThread()\n");
	Thread *t=(Thread *) ptr;
	t->run();
	printf("Thread::startupThread() done\n");
	return NULL;
}

void Thread::start() {
	int s = pthread_create(&this->thread, NULL, &Thread::startupThread, (void *) this);
	if (s != 0)
		perror("pthread_create");
}
