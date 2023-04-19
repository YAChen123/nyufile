CC=gcc
CFLAGS= -g -pedantic -std=gnu17 -Wall -Werror -Wextra -Wno-unused

.PHONY: all
all: clean nyufile

nyufile: nyufile.o recover.o
	$(CC) $(CFLAGS) nyufile.o recover.o -o nyufile

nyufile.o: nyufile.c recover.h
	$(CC) $(CFLAGS) -c nyufile.c

recover.o: recover.c recover.h
	$(CC) $(CFLAGS) -c recover.c

.PHONY: clean
clean:
	rm -f *.o nyufile new_input.txt *.enc

