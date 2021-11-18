#define _GNU_SOURCE  // for asprintf
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/signal.h>
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


int checkArrayForInteger(int value, int *arr, int arr_length);
char* format_string(char*msg, int data);
int is_diff_gt_second(Clock start, Clock current);

int main(int argc, char *argv[]) {
  printf("In user!\n");

  srand(time(NULL) + getpid()); // re-seed the random

  if(argc != 4) {
    perror("oss: user: Usage: `./user b s p`, where b is an integer and s is the shared memory id and p is the simulated pid\n");
    return 0;
  }
  char *b_str = argv[1];
  printf("b from params: %s\n", b_str);

  if(!atoi(b_str)) {
    perror("oss: user: Error: paramter received for 'B' is not a number\n");
    return 0;
  }
  int b = atoi(b_str);

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
    return 0;
  }

  // get timestamp for beginning of process run
  Clock begin_run;
  begin_run.sec = resource_table->clock.sec;
  begin_run.ns = resource_table->clock.ns;

  // use for sending back through termination
  Clock end_run;
  end_run.sec = begin_run.sec;
  end_run.ns = begin_run.ns;

  // TODO: MAKE SURE REQUEST NEVER EXCEEDS TOTAL AMOUNT AVAILABLE IN RESOURCE_TABLE
  // --> total = (request + allocation)


  int should_terminate = 0; // 1 for true

  while(should_terminate != 1) {
    printf("\nUser.c in should_terminate loop!\n");

    mymsg_t mymsg;

    // AFTER BEING SCHEDULED...
    // generate random from [0, B], where B is the upper bound for when a process should request a new resource
    int next_request = getRandom(b + 1);
    printf("user: next request: %d\n", next_request);

    // int next_request_us = next_request * MS_TO_US;
    // usleep(next_request_us);

    // add request to end_run
    int next_request_ns = next_request * MS_NS_CONVERSION;
    if(end_run.ns + next_request_ns >= NANOSECONDS) {
      int remaining_ns = (end_run.ns + next_request_ns) - NANOSECONDS;
      end_run.sec += 1;
      end_run.ns = remaining_ns;
    }
    else
      end_run.ns += next_request_ns;

    // Decide if it should request or release
    int should_request = getRandom(5);

    // Request
    if(should_request > 1) { // buf_term, MAX_MSG_SIZE, msg_type, resource_table->queueid
      // Create Request for Resources
      int resource_index_requested = getRandom(RESOURCES_DEFAULT); // index of resource requested
      // printf("requesting index: %d\n", resource_index_requested);
      int n_resources = resource_table->resources[resource_index_requested].n_resources;
      // printf("n resources: %d\n", n_resources);
      int requested_resources = getRandom(n_resources) + 1;
      // printf("requested res amount: %d\n", requested_resources);

      char *buf;
      int msg_type = 1; // 1 for request
      asprintf(&buf, "%d-%d-%d-%d-%d-%d-", pid, msg_type, resource_index_requested, requested_resources, end_run.sec, end_run.ns);

      fprintf(stderr, "user: about to send message (request): %s\n", buf);

      // write to message buf: resource_index_requested, resource_request_amount
      // int buf_size = strlen(buf);

      if((size = msgwrite(buf, MAX_MSG_SIZE, msg_type, resource_table->queueid)) == -1) {
        perror("oss: user: Error: could not send message from user to oss");
        exit(0);
        return 0;
      }

      // wait to receive message (see if approved)
      int msg_size = msgrcv(resource_table->queueid, &mymsg, MAX_MSG_SIZE, 4, 0);
      if(msg_size == -1) {
        perror("oss: user: Error: Could not receive message from parent\n");
        return 0;
      }
      
      printf("user: Request: got msg from parent!\n");

      // check if approved
      int request_approved;
      char *request_approved_str = strtok(mymsg.mtext, "-");
      request_approved = atoi(request_approved_str);

      if(request_approved == 0) {
        printf("request for resources not approved (got 0)\n");
      }
      else {
        printf("request for resources approved (got 1)\n");
      }
    }
    // Release
    else {
      // Create Release for resources
      int resource_index_requested = getRandom(RESOURCES_DEFAULT); // index of resource requested
      // printf("requesting index: %d\n", resource_index_requested);
      int n_resources = resource_table->resources[resource_index_requested].n_resources;
      // printf("n resources: %d\n", n_resources);
      int requested_resources = getRandom(n_resources) + 1;
      // printf("requested res amount: %d\n", requested_resources);

      char *buf;
      int msg_type = 2; // 2 for release
      asprintf(&buf, "%d-%d-%d-%d-%d-%d-", pid, msg_type, resource_index_requested, requested_resources, end_run.sec, end_run.ns);

      fprintf(stderr, "user: about to send message (release): %s\n", buf);

      // write to message buf: resource_index_requested, resource_request_amount
      // int buf_size = strlen(buf);

      if((size = msgwrite(buf, MAX_MSG_SIZE, msg_type, resource_table->queueid)) == -1) {
        perror("oss: user: Error: could not send message from user to oss");
        exit(0);
        return 0;
      }

      int msg_size = msgrcv(resource_table->queueid, &mymsg, MAX_MSG_SIZE, 5, 0);
      if(msg_size == -1) {
        perror("oss: user: Error: Could not receive message from parent\n");
        return 0;
      }
      
      printf("user: Release: got msg from parent: %s\n", mymsg.mtext);
    }

    // schedule to ask for the resources
    // WHEN it does get the request approved,
    // - either request resources
    // - or release resources (if previously requested)
    // - continue in a loop in this fashion until termination


    // the process checks if it should terminate in random intervals [0,250] milliseconds
    int next_termination;
    if(is_diff_gt_second(begin_run, end_run) == 1)
      next_termination = getRandom(251); // 0-250
    else
      next_termination = 1; // dont terminate if < 1 sec has ran

    if(next_termination > 200) {
      // should terminate
      fprintf(stderr, "terminating child\n");
      should_terminate = 1;

      // add on the ms to terminate
      int next_termination_ns = next_termination * MS_NS_CONVERSION;
      if(end_run.ns + next_termination_ns >= NANOSECONDS) {
        int remaining_ns = (end_run.ns + next_termination_ns) - NANOSECONDS;
        end_run.sec += 1;
        end_run.ns = remaining_ns;
      }
      else
        end_run.ns += next_termination_ns;
    }
  } // END While


  // IF it should terminate,
    // - release all resources allocated to it by communicating to oss that it is releasing the resources
    // - do this only after it has run for at least 1 second
    // - do this by putting a request in shared memory for oss to pick up
    // - the request should never exceed the total number of resources of that class within the system
    // - update the system clock

  // send message to oss
  int msg_type = 3; // 3 for termination
  char *buf_term;

  fprintf(stderr, "(user) Ready to print message to terminate\n");

  // Message: PID-TYPE-SEC-NS
  asprintf(&buf_term, "%d-%d-%d-%d-", pid, msg_type, end_run.sec, end_run.ns);

  if((size = msgwrite(buf_term, MAX_MSG_SIZE, msg_type, resource_table->queueid)) == -1) {
    perror("oss: user: Error: could not send message from user to oss\n");
    return 0;
  }
  else printf("Message to terminate sent successfully");

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

// return 1 for true, 0 for false
int is_diff_gt_second(Clock start, Clock current) {
  int diff_sec = current.sec - start.sec;
  if(diff_sec > 1)
    return 1;
  if(diff_sec == 0)
    return 0;

  int diff_ns = current.ns - start.ns;
  // if diff_sec == 1, we need to see if there's enough on both sides to make NS > NANOSECONDS
  int diff_ns_start = NANOSECONDS - start.ns;
  
  if(diff_ns_start + current.ns > NANOSECONDS)
    return 1;

  return 0;
}