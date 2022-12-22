.PHONY: all
all: starter writer reader

starter: starter.c
	gcc -Wall -o starter starter.c

writer: writer.c
	gcc -Wall -o writer writer.c

reader: reader.c
	gcc -Wall -o reader reader.c -lrt

.PHONY: clean
clean:
	rm -f starter writer reader
