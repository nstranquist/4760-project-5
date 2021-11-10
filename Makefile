CC = gcc
.SUFFIXES: .c .o .h

all: oss user

oss: oss.o user.o resource_table.o semaphore_manager.o config.h
	gcc -Wall -g -o oss oss.o resource_table.o semaphore_manager.o

user: user.o
	gcc -Wall -g -o user user.o

resource_table: resource_table.o
	gcc -Wall -g -o resource_table resource_table.o

semaphore_manager: semaphore_manager.o
	gcc -Wall -g -o semaphore_manager semaphore_manager.o

.c.o:
	$(CC) -g -c $<

clean:
	rm -f *.o oss user