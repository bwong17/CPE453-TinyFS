CC = gcc
FLAGS = -Wall -g 
PROG = tinyFsDemo
OBJS = tinyFsDemo.o libTinyFS.o libDisk.o

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) -lm $(OBJS)

tinyFsDemo.o: tinyFsDemo.c libTinyFS.h tinyFS.h TinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $< -lm

libTinyFS.o: libTinyFS.c libTinyFS.h tinyFS.h libDisk.h libDisk.o TinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $< -lm

libDisk.o: libDisk.c libDisk.h tinyFS.h TinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $< -lm
