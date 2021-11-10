#ifndef SEMAPHORE_MANAGER_H
#define SEMAPHORE_MANAGER_H

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <errno.h>
#include "config.h"


int initelement(int semid, int semnum, int semvalue);
void setsembuf(struct sembuf *s, int num, int op, int flg);
void wait_sem(int semid, struct sembuf *sops, size_t nsops);
void signal_sem(int semid, struct sembuf *sops, size_t nsops);
int removesem(int semid);
int r_semop(int semid, struct sembuf *sops, int nsops);


#endif