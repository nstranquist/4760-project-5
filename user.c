#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/shm.h>
#include "config.h"
#include "utils.h"
#include "resource_table.h"

extern struct ResourceTable *resource_table;
int size;
int shmid;

int main(int argc, char *argv[]) {
  printf("In user!");

  srand(time(NULL) + getpid()); // re-seed the random

  if(argc != 2) {
    perror("user: Usage: ./user b\n");
    return 1;
  }
  char *b_str = argv[1];
  printf("b from params: %s\n", b_str);

  if(!atoi(b_str)) {
    perror("user: Error: paramter received for 'B' is not a number\n");
    return 1;
  }

  // attach shared memory
  resource_table = (ResourceTable *)shmat(shmid, NULL, 0);
  if (resource_table == (void *) -1) {
    perror("oss: Error: Failed to attach to shared memory\n");
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
      perror("oss: Error: Failed to remove memory segment\n");
    return -1;
  }

  int b = atoi(b_str);

  // generate random from [0, B], where B is the upper bound for when a process should request a new resource
  // is B milliseconds or nanoseconds?
  int next_request = getRandom(b + 1);
  printf("next request: %d\n", next_request);



  // schedule to ask for the resources




  // release the resources


  return 0;
}
