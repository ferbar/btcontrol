#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#define GPIO_FN_MAXLEN	32
#define POLL_TIMEOUT	1000
#define RDBUF_LEN	5

int main(int argc, char **argv) {
	char fn[GPIO_FN_MAXLEN];
	int fd,ret;
	struct pollfd pfd;
	char rdbuf[RDBUF_LEN];

	struct timespec gettime_timespec;

	double gettime_start=0.0, gettime_now=0.0;

	memset(rdbuf, 0x00, RDBUF_LEN);
	memset(fn, 0x00, GPIO_FN_MAXLEN);

	if(argc!=2) {
		printf("Usage: %s <GPIO>\nGPIO must be exported to sysfs and have enabled edge detection\n", argv[0]);
		return 1;
	}
	snprintf(fn, GPIO_FN_MAXLEN-1, "/sys/class/gpio/gpio%s/value", argv[1]);
	fd=open(fn, O_RDONLY);
	if(fd<0) {
		perror(fn);
		return 2;
	}
	pfd.fd=fd;
	pfd.events=POLLPRI;
	
	ret=read(fd, rdbuf, RDBUF_LEN-1);
	if(ret<0) {
		perror("read()");
		return 4;
	}
	printf("value is: %s\n", rdbuf);
	
	while(1) {
		memset(rdbuf, 0x00, RDBUF_LEN);
		lseek(fd, 0, SEEK_SET);
		ret=poll(&pfd, 1, POLL_TIMEOUT);
		if(ret<0) {
			perror("poll()");
			close(fd);
			return 3;
		}
		if(ret==0) {
			printf("timeout\n");
			continue;
		}
		ret=read(fd, rdbuf, RDBUF_LEN-1);
		if(ret<0) {
			perror("read()");
			return 4;
		}
		if(gettime_start != 0) {
			clock_gettime(CLOCK_REALTIME, &gettime_timespec);
			gettime_now=gettime_timespec.tv_sec+(gettime_timespec.tv_nsec/1000000000.0);
			printf("time: %f\n",gettime_now-gettime_start);
		}
		printf("interrupt, value is: %s (time:)\n", rdbuf);
		clock_gettime(CLOCK_REALTIME, &gettime_timespec);
		gettime_start=gettime_timespec.tv_sec+(gettime_timespec.tv_nsec/1000000000.0);
	}
	close(fd);
	return 0;
}
