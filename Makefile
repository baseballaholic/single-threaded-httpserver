CC = clang
CFLAGS = -Wall -Wpedantic -Werror -Wextra

all: httpserver

httpserver: httpserver.o bind.o
	$(CC) -o httpserver httpserver.o bind.o

httpserver.o: httpserver.c bind.h
	$(CC) $(CFLAGS) -c httpserver.c

bind.o: bind.h bind.c
	$(CC) $(CFLAGS) -c bind.c

clean: rm -f httpserver httpserver.o
