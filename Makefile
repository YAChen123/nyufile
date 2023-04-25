CC=gcc
CFLAGS= -g -pedantic -std=gnu17 -Wall -Werror -Wextra -Wno-unused -I/usr/local/opt/openssl/include
LDFLAGS= -L/usr/local/opt/openssl/lib


.PHONY: all
all: clean nyufile

nyufile: nyufile.o recover.o
	$(CC) $(CFLAGS) $(LDFLAGS) nyufile.o recover.o -o nyufile -lcrypto

nyufile.o: nyufile.c recover.h
	$(CC) $(CFLAGS) -c nyufile.c

recover.o: recover.c recover.h
	$(CC) $(CFLAGS) -c recover.c

.PHONY: clean
clean:
	rm -f *.o nyufile new_input.txt *.enc

