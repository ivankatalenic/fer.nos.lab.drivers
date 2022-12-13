/* simple program that uses ioctl to send a command to given file */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int fd;
	ssize_t count;

	if (argc < 3) {
		fprintf(stderr, "Usage: %s file-name data\n", argv[0]);
		return -1;
	}

	fd = open(argv[1], O_WRONLY);
	if (fd == -1) {
		perror("open failed");
		return -1;
	}

	size_t size = strlen(argv[2]);
	count = write(fd, argv[2], size);
	if (count == -1) {
		perror("write error");
		return -1;
	}

	printf("write returned %ld\n", count);

	return 0;
}
