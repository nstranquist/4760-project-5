CC = gcc
.SUFFIXES: .c .o .h

all: oss user

oss: oss.o user.o resource_table.o semaphore_manager.o utils.o deadlock_detection.o config.h
	gcc -Wall -g -o oss oss.o resource_table.o semaphore_manager.o utils.o deadlock_detection.o

user: user.o utils.o resource_table.o
	gcc -Wall -g -o user user.o utils.o resource_table.o

resource_table: resource_table.o utils.o
	gcc -Wall -g -o resource_table resource_table.o utils.o

semaphore_manager: semaphore_manager.o
	gcc -Wall -g -o semaphore_manager semaphore_manager.o

utils: utils.o
	gcc -Wall -g -o utils utils.o

deadlock_detection: deadlock_detection.o
	gcc -Wall -g -o deadlock_detection deadlock_detection.o

.c.o:
	$(CC) -g -c $<

clean:
	rm -f *.o oss user resource_table semaphore_manager utils