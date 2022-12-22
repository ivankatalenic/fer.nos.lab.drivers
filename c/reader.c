#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define CLOCKID CLOCK_REALTIME
#define SIG SIGTERM

#define errExit(msg) { perror(msg); exit(EXIT_FAILURE); }

static char* id;
static int   fd;

static void signal_handler(int sig, siginfo_t *si, void *uc) {
	printf("closing reader %s\n", id);
	close(fd);
	exit(0);
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		fprintf(stderr, "there should be three arguments, got %d\n", argc - 1);
		return EXIT_FAILURE;
	}

	id = argv[1];
	char* filename = argv[2];

	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = signal_handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIG, &sa, NULL) == -1)
		errExit("sigaction");

	struct sigevent sev;
	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIG;
	timer_t timerid;
	if (timer_create(CLOCKID, &sev, &timerid) == -1)
		errExit("timer_create");

	struct itimerspec its;
	its.it_value.tv_sec = 5;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = its.it_value.tv_sec;
	its.it_interval.tv_nsec = its.it_value.tv_nsec;
	if (timer_settime(timerid, 0, &its, NULL) == -1)
		errExit("timer_settime");

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		perror("open failed");
		return -1;
	}

	while (1) {
		#define count 256
		char buf[count];

		memset(buf, 0, count * sizeof(char));

		ssize_t nread = read(fd, buf, count);
		if (nread == -1) {
			perror("read error");
			return -1;
		}

		printf("reader %s: \"%s\"\n", id, buf);
	}

	return 0;
}
