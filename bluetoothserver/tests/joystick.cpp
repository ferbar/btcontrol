#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <linux/joystick.h>
#include <unistd.h>


int main(int argv, const char **argc) {
	printf("test\n");
	int fd = open("/dev/input/js0", O_RDONLY);
	if(fd <= 0) {
		printf("error open js\n");
		exit(1);
	}
	struct js_event e;
	while( read (fd, &e, sizeof(e)) > 0) {
		printf("%d %d %d %d\n", e.time, e.value, e.type, e.number);
	}
}
