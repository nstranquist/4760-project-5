#include "semaphore_manager.h"

// Initialize the sempahore element
int initelement(int semid, int semnum, int semvalue) {
  union semun {
    int val;
    struct semids_ds *buf;
    unsigned short *array;
  } arg;

  arg.val = semvalue;

  return semctl(semid, semnum, SETVAL, arg);
}

// initializes the struct sembuf structure members sem_num, sem_op, and sem_flg
void setsembuf(struct sembuf *s, int num, int op, int flg) {
  s->sem_num = (short)num;
  s->sem_op = (short)op;
  s->sem_flg = (short)flg;
  return;
}

// restarts the semop if interrupt received
int r_semop(int semid, struct sembuf *sops, int nsops) {
  while(semop(semid, sops, nsops) == -1) {
    if(errno != EINTR)
      return -1;
  }
  return 0;
}


// removed the semaphore specified by semid
int removesem(int semid) {
  return semctl(semid, 0, IPC_RMID);;
}

void wait_sem(int semid, struct sembuf *sops, size_t nsops) {
  while(r_semop(semid, sops, nsops) == -1) {
    perror("runsim: Error: Failed to enter critical section (semop wait)\n");
    exit(1);
  }
}

void signal_sem(int semid, struct sembuf *sops, size_t nsops) {
  while(r_semop(semid, sops, nsops) == -1) {
    perror("runsim: Error: Failed to exit critical section (semop signal)\n");
    exit(1);
  }
}
