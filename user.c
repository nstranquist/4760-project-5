#define _GNU_SOURCE  // for asprintf
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/shm.h>
// #include "config.h"
#include "utils.h"
#include "resource_table.h"
#include "queue.h"
#include "circular_queue.h"
// #include "semaphore_manager.h"

extern ResourceTable *resource_table;
int size;
int shmid;
int pid;
// semaphore structures here, if needed

mymsg_t mymsg;

int checkArrayForInteger(int value, int *arr, int arr_length);
char* format_string(char*msg, int data);

int main(int argc, char *argv[]) {
  printf("In user!\n");

  srand(time(NULL) + getpid()); // re-seed the random

  if(argc != 4) {
    perror("oss: user: Usage: `./user b s p`, where b is an integer and s is the shared memory id and p is the simulated pid\n");
    return 1;
  }
  char *b_str = argv[1];
  printf("b from params: %s\n", b_str);

  if(!atoi(b_str)) {
    perror("oss: user: Error: paramter received for 'B' is not a number\n");
    return 1;
  }

  if(!atoi(argv[2])) {
    fprintf(stderr, "oss: user: Error: argument for shmid must be a valid integer\n");
  }
  shmid = atoi(argv[2]);

  fprintf(stderr, "shmid: %d\n", shmid);

  if(!atoi(argv[3])) {
    fprintf(stderr, "oss: user: Error: argument for pid must be a valid integer\n");
  }
  pid = atoi(argv[3]);

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
  int next_request = getRandom(b + 1);
  printf("next request: %d\n", next_request);

  // never more than 5 instances of the tape drive (# instances available in the system)

  // # units available at the address of the pointer

  // ask for resource to allocate

  // what information is needed to communicate properly with oss?
  // what information is in a given resource descriptor?
  // send test message to oss
  char *buf = "";

  long msg_type = 1;
  int resource_index_requested = getRandom(RESOURCES_DEFAULT); // index of resource requested
  printf("requesting index: %d\n", resource_index_requested);
  int n_resources = resource_table->resources[resource_index_requested].n_resources;
  printf("n resources: %d\n", n_resources);
  int requested_resources = getRandom(n_resources) + 1;
  printf("requested res amount: %d\n", requested_resources);
  // int resource_request_amount = getRandom(RESOURCES_DEFAULT-1) + 1;
  // printf("# requested: %d\n", resource_request_amount);

  buf = format_string(buf, pid);
  strcat(buf, "-");
  buf = format_string(buf, 0); // 0 is for request type
  strcat(buf, "-");
  buf = format_string(buf, resource_index_requested);
  strcat(buf, "-");
  buf = format_string(buf, requested_resources);
  strcat(buf, "-");
  // add time in sec and ns
  buf = format_string(buf, resource_table->clock.sec);
  strcat(buf, "-");
  buf = format_string(buf, resource_table->clock.ns);
  strcat(buf, "-");

  fprintf(stderr, "about to send message\n");

  // write to message buf: resource_index_requested, resource_request_amount


  if((size = msgwrite(buf, MAX_MSG_SIZE, msg_type, resource_table->queueid)) == -1) {
    perror("oss/user: Error: could not send message from user to oss\n");
    return 1;
  }
  else {
    printf("sent message back to oss from user\n");
  }
  

  // sleep to test concurrency in oss
  // sleep(0.2);

  // schedule to ask for the resources
  // WHEN it does get the request approved,
  // - either request resources
  // - or release resources (if previously requested)
  // - continue in a loop in this fashion until termination


  // the process checks if it should terminate in random intervals [0,250] milliseconds

  // IF it should terminate,
  // - release all resources allocated to it by communicating to oss that it is releasing the resources
  // - do this only after it has run for at least 1 second
  // - do this by putting a request in shared memory for oss to pick up
  // - the request should never exceed the total number of resources of that class within the system
  // - update the system clock
  // - the process may decide to give up resources instead of asking for them



  // release the resources



  fprintf(stderr, "user: Child is exiting\n");
  return 0;
}


int checkArrayForInteger(int value, int *arr, int arr_length) {
  for(int i = 0; i<arr_length; i++) {
    if(arr[i] == value)
      return -1;
  }
  return 0;
}

char* format_string(char*msg, int data) {
  char *temp;
  char *buf;
  if (asprintf(&temp, "%d", data) == -1) {
    perror("oss: Warning: string format failed\n");
    return "";
  } else {
    strcat(strcpy(buf, msg), temp);
    free(temp);
    return buf;
  }
}