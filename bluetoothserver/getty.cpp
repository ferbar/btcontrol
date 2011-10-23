#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>

int main(int argc, const char* argv[]) {
	int flags;
	int fd=open("/dev/ttyS0",O_RDONLY);
	ioctl(fd, TIOCMGET, &flags );
	if ( flags & TIOCM_RTS ) puts(" RTS");
	if ( flags & TIOCM_CTS ) puts(" CTS");
	if ( flags & TIOCM_DSR ) puts(" DSR");
	if ( flags & TIOCM_DTR ) puts(" DTR");
//	if ( flags & TIOCM_DCD ) puts(" DCD");
	if ( flags & TIOCM_RI  ) puts(" RI");
	return 0;
}
