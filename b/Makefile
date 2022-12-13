.PHONY: all
all: ioctl write read

ioctl: ioctl.c
	gcc -Wall -o ioctl ioctl.c

write: write.c
	gcc -Wall -o write write.c

read: read.c
	gcc -Wall -o read read.c

.PHONY: clean
clean:
	rm -f ioctl write read
