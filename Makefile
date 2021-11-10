CC = gcc
.SUFFIXES: .c .o .h

all: oss user

oss: oss.o user.o config.h
	gcc -Wall -g -o oss oss.o

user: user.o
	gcc -Wall -g -o user user.o

.c.o:
	$(CC) -g -c $<

clean:
	rm -f *.o oss user