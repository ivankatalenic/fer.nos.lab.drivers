#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include "utils.h"

#define errExit(msg) { perror(msg); exit(EXIT_FAILURE); }

int main(int argc, char* argv[]) {
	int writers = 6;
	if (argc == 2) {
		writers = atoi(argv[1]);
		if (writers < 1) {
			fprintf(stderr, "The number of devices has to be greater than 0, received %d\n", writers);
			return EXIT_FAILURE;
		}
	}

	const int nfds = writers;
	int open_fds = nfds;
	struct pollfd *pfds;

	pfds = calloc(nfds, sizeof(struct pollfd));
	if (pfds == NULL) {
		errExit("calloc");
	}

	for (int i = 0; i < nfds; i++) {
		char filename[32];
		sprintf(filename, "/dev/shofer%d", i);
		pfds[i].fd = open(filename, O_WRONLY);
		if (pfds[i].fd == -1) {
			errExit("open");
		}

		pfds[i].events = POLLOUT;
	}

	while (open_fds > 0) {
		const int ready = poll(pfds, nfds, -1);
		if (ready == -1) {
			errExit("poll");
		}

		printf("Ready: %d\n", ready);

		// Check for errors
		int err_cnt = 0;
		for (int i = 0; i < nfds; i++) {
			if (pfds[i].revents & POLLERR || pfds[i].revents & POLLHUP) {
				printf("Closing fd %d\n", i);
				if (close(pfds[i].fd) == -1) {
					errExit("close");
				}
				open_fds--;
				err_cnt++;
			}
		}

		if (err_cnt == ready) {
			// All events are errors, none for writing
			continue;
		}

		// Find the FD to write to
		const int selected = rand_int(1, writers);
		int idx;
		int found = 0;
		for (idx = 0; 1 == 1; idx = (idx + 1) % writers) {
			if (pfds[idx].revents & POLLOUT) {
				found++;
				if (found == selected) {
					break;
				}
			}
		}

		// Write the character
		char buf = 'F';
		ssize_t ret = write(pfds[idx].fd, &buf, sizeof(buf));
		if (ret == -1) {
			errExit("write");
		}
		printf("Wrote to device %d char: %c\n", idx, buf);

		sleep_ms(2000);
	}

	return EXIT_SUCCESS;
}
