#include <stdlib.h>
#include <time.h>
#include <threads.h>

int rand_int(int low, int high) {
	if (low >= high) {
		return 0;
	}

	struct timespec ts;
	timespec_get(&ts, TIME_UTC);

	srand(ts.tv_nsec);

	return low + rand() / ((RAND_MAX + 1u) / (high - low + 1));
}

void sleep_ms(long duration) {
	if (duration <= 0) {
		return;
	}

	time_t sec  = 0;
	long   nsec = 0;

	if (duration > 1000) {
		sec = duration / 1000;
	}
	nsec = (duration % 1000) * 1000 * 1000;

	struct timespec ts = {.tv_sec = sec, .tv_nsec = nsec};
	thrd_sleep(&ts, NULL);
}

