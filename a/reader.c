#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#define errExit(msg) { perror(msg); exit(EXIT_FAILURE); }

int main(int argc, char* argv[]) {
	int readers = 6;
	if (argc == 2) {
		readers = atoi(argv[1]);
		if (readers < 1) {
			fprintf(stderr, "The number of devices has to be greater than 0, received %d\n", readers);
			return EXIT_FAILURE;
		}
	}

	const int nfds = readers;
	int open_fds = nfds;
	struct pollfd *pfds;

	pfds = calloc(nfds, sizeof(struct pollfd));
	if (pfds == NULL) {
		errExit("calloc");
	}

	for (int i = 0; i < nfds; i++) {
		char filename[32];
		sprintf(filename, "/dev/shofer%d", i);
		pfds[i].fd = open(filename, O_RDONLY);
		if (pfds[i].fd == -1) {
			errExit("open");
		}

		pfds[i].events = POLLIN;
	}

	while (open_fds > 0) {
		const int ready = poll(pfds, nfds, -1);
		if (ready == -1) {
			errExit("poll");
		}

		printf("Ready: %d\n", ready);

		for (int i = 0; i < nfds; i++) {
			if (pfds[i].revents & POLLIN) {
				char buf;
				ssize_t ret = read(pfds[i].fd, &buf, sizeof(buf));
				if (ret == -1) {
					errExit("read");
				}
				printf("Read from device %d char: %c\n", i, buf);
			}
			if (pfds[i].revents & POLLERR || pfds[i].revents & POLLHUP) {
				printf("Closing fd %d\n", i);
				if (close(pfds[i].fd) == -1) {
					errExit("close");
				}
				open_fds--;
			}
		}
	}

	return EXIT_SUCCESS;
}
