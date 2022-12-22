#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
	int readers;
	int writers;

	if (argc != 5) {
		fprintf(stderr, "starter <filename> <readers> <writers> <attempts>\n");
		return EXIT_FAILURE;
	}

	readers = atoi(argv[2]);
	writers = atoi(argv[3]);
	
	// Create reader processes
	int i;
	for (i = 1; i <= readers; i++) {
		char id[4];
		sprintf(id, "%d", i);

		int ret;
		int status = fork();
		switch (status) {
			case -1:
				fprintf(stderr, "can't create a new reader process\n");
				return EXIT_FAILURE;
			case 0:
				ret = execl("./reader", "reader", id, argv[1], argv[4], NULL);
				if (ret == -1) {
					fprintf(stderr, "can't run the reader process\n");
					return EXIT_FAILURE;
				}
				break;
		}
	}

	// Create writer processes
	for (i = 1; i <= writers; i++) {
		char id[4];
		sprintf(id, "%d", i);

		int ret;
		int status = fork();
		switch (status) {
			case -1:
				fprintf(stderr, "can't create a new writer process\n");
				return EXIT_FAILURE;
			case 0:
				ret = execl("./writer", "writer", id, argv[1], argv[4], NULL);
				if (ret == -1) {
					fprintf(stderr, "can't run the writer process\n");
					return EXIT_FAILURE;
				}
				break;
		}
	}

	int pid = 0;
	i = 0;
	while (pid != -1) {
		pid = wait(NULL);
		i++;
	}
	printf("wait called %d times\n", i);

	return EXIT_SUCCESS;
}

