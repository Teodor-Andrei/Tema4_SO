CC = gcc
CFLAGS = -Wall
LDLIBS = -lpthread
LDFLAGS = -L.

build: main

main: main.o so_scheduler.o

main.o: main.c 

so_scheduler.o: so_scheduler.c so_scheduler.h utils.h

.PHONY:
clean:
	rm -f *.o main