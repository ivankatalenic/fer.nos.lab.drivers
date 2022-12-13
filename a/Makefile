reader: reader.o
	gcc -o reader reader.o

reader.o: reader.c
	gcc -Wall -c reader.c

writer: writer.o utils.o
	gcc -o writer writer.o utils.o

writer.o: writer.c utils.h
	gcc -Wall -c writer.c

utils.o: utils.c utils.h
	gcc -Wall -c utils.c

