#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	if (argc != 4) {
		fprintf(stderr, "there should be three arguments, got %d\n", argc - 1);
		return EXIT_FAILURE;
	}

	char* id = argv[1];
	char* filename = argv[2];
	int attempts = atoi(argv[3]);

	int fd;
	ssize_t count;

	fd = open(filename, O_WRONLY);
	if (fd == -1) {
		perror("open failed");
		return -1;
	}

	for (int i = 0; i < attempts; i++) {
		printf("writer %s: attempting %d\n", id, i + 1);
		char buf[256];
		sprintf(buf, "writer %s, attempt %d", id, i + 1);
		size_t size = strlen(buf);
		count = write(fd, buf, size);
		if (count == -1) {
			perror("write error");
			return -1;
		}
		printf("writer %s: succeeded at attempt %d\n", id, i + 1);
	}

	close(fd);

	return 0;
}
