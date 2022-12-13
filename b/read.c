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
	ssize_t nread;
	ssize_t count;
	char* buf;

	if (argc < 3) {
		fprintf(stderr, "Usage: %s file-name count\n", argv[0]);
		return -1;
	}

	count = atol(argv[2]);
	if (count < 1) {
		fprintf(stderr, "Usage: %s file-name count\n", argv[0]);
		fprintf(stderr, "count must be an integer greater than 0\n");
		return -1;
	}

	buf = calloc(count, sizeof(char));
	if (buf == NULL) {
		perror("calloc failed");
		return -1;
	}

	fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		perror("open failed");
		free(buf);
		return -1;
	}

	nread = read(fd, buf, count);
	if (nread == -1) {
		perror("read error");
		free(buf);
		return -1;
	}

	printf("read returned (%ld): \"%s\"\n", nread, buf);
	free(buf);

	return 0;
}
