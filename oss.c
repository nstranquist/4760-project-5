#define _GNU_SOURCE  // for asprintf
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
// #include <time.h>
// #include <wait.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <getopt.h>
#include <stdbool.h>
#include <semaphore.h>
#include <math.h> // for randomness
#include "config.h"
#include "resource_table.h"
#include "semaphore_manager.h"
#include "utils.h"
#include "deadlock_detection.h"
#include "queue.h"
#include "circular_queue.h"

#define PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)


// global variables
extern ResourceTable *resource_table;
const char* logfile = "oss.log";
int shmid; // to manage shared memory
int semid; // to manage semaphore
struct sembuf semsignal[1];
struct sembuf semwait[1];

Clock next_fork;
Clock time_diff; // keep track of last round's time difference

Queue wait_queue;

int available_pids[MAX_PROCESSES_TOTAL];


// function definitions
int detachandremove(int shmid, void *shmaddr);
void logmsg(const char *msg);
void cleanup();
void generate_report();
void print_current_resources();
void generate_next_child_fork();
int wait_time_is_up(); // compares next_fork with Clock's current time
int check_line_count(FILE *fp);
Clock increment_clock();
void init_pids();
int getNextPid();
void freePid(int pid);
int found_in_resource_array(int resource_index, int process_index);

static void myhandler(int signum) {
  // is ctrl-c interrupt
  if(signum == SIGINT)
    perror("\noss: Ctrl-C Interrupt Detected. Shutting down gracefully...\n");
  // is timer interrupt
  else if(signum == SIGALRM)
    perror("\noss: Info: The time for this program has expired. Shutting down gracefully...\n");
  else {
    perror("\noss: Warning: Only Ctrl-C and Timer signal interrupts are being handled.\n");
    return; // ignore the interrupt, do not exit
  }

  fprintf(stderr, "interrupt handler\n");

  generate_report();

  cleanup();
  
  pid_t group_id = getpgrp();
  if(group_id < 0)
    perror("oss: Info: group id not found\n");
  else
    killpg(group_id, signum);


  kill(getpid(), SIGKILL);
	exit(0);
  signal(SIGQUIT, SIG_IGN);
}


// interrupt handling
static int setupitimer(int sleepTime) {
  struct itimerval value;
  value.it_interval.tv_sec = 0;
  value.it_interval.tv_usec = 0;
  value.it_value.tv_sec = sleepTime; // alarm
  value.it_value.tv_usec = 0;
  return (setitimer(ITIMER_PROF, &value, NULL));
}

static int setupinterrupt(void) {
  struct sigaction act;
  act.sa_handler = myhandler;
  act.sa_flags = 0;
  return (sigemptyset(&act.sa_mask) || sigaction(SIGPROF, &act, NULL) || sigaction(SIGALRM, &act, NULL));
}

static int timerHandler(int s) {
  int errsave;
  errsave = errno;
  write(STDERR_FILENO, "The time limit was reached\n", 1);
  errno = errsave;
}

int main(int argc, char*argv[]) {
  printf("Starting oss... no parameters required\n");

  int nextFork;

  // setup timers and interrupts
  if (setupinterrupt() == -1) {
    perror("oss: Error: Could not run setup the interrupt handler.\n");
    return -1;
  }
  if (setupitimer(MAX_SECONDS) == -1) {
    perror("oss: Error: Could not setup the interval timer.\n");
    return -1;
  }

  // Setup intterupt handler
  signal(SIGINT, myhandler);

  // setup logfile
  // Test that logfile can be used
  FILE *fp = fopen(logfile, "w");
  if(fp == NULL) {
    perror("oss: Error: Could not open log file for writing.\n");
    return 1;
  }
  fprintf(fp, "Log Info for OSS Program:\n"); // clear the logfile to start
  fclose(fp);

  // seed the random
  srand(time(NULL));

  // allocate shared memory
  shmid = shmget(IPC_PRIVATE, sizeof(ResourceTable), PERMS | 0666);
  if (shmid == -1) {
    perror("oss: Error: Failed to create shared memory segment for process table\n");
    return -1;
  }

  // attach shared memory
  resource_table = (ResourceTable *)shmat(shmid, NULL, 0);
  if (resource_table == (void *) -1) {
    perror("oss: Error: Failed to attach to shared memory\n");
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
      perror("oss: Error: Failed to remove memory segment\n");
    return -1;
  }

  // Initialize Message Queue
  int queueid = initqueue(IPC_PRIVATE);
  if(queueid == -1) {
    perror("oss: Error: Failed to initialize message queue\n");
    cleanup();
    return -1;
  }
  resource_table->queueid = queueid;

  // Create semaphore containing a single element
  if((semid = semget(IPC_PRIVATE, 1, PERMS)) == -1) {
    perror("oss: Error: Failed to create private semaphore\n");
    return 1;
  }

  setsembuf(semwait, 0, -1, 0); // decrement first element of semwait
  setsembuf(semsignal, 0, 1, 0); // increment first element of semsignal

  // initialize semaphore before use
  if(initelement(semid, 0, 1) == -1) {
    perror("oss: Error: Failed to init semaphore element value to 1\n");
    if(removesem(semid) == -1)
      perror("oss: Error: Failed to remove failed semaphore\n");
    return 1;
  }

  // Initialize Process Queues
  wait_queue = init_circular_queue();

  // Start program timer
  alarm(MAX_SECONDS);

  
  // init clock as cs
  wait_sem(semid, semwait, 1);
  resource_table->clock.sec = 0;
  resource_table->clock.ns = 0;
  signal_sem(semid, semsignal, 1);

  // init resource table as cs
  wait_sem(semid, semwait, 1);
  initialize_resource_table();
  signal_sem(semid, semsignal, 1);

  init_pids();

  // print_resources();

  // generate next time as cs
  wait_sem(semid, semwait, 1);
  generate_next_child_fork();
  signal_sem(semid, semsignal, 1);

  printf("next fork in: %d sec, %d ns\n", next_fork.sec, next_fork.ns);

  // start process loop (main logic)
  while(resource_table->total_processes < MAX_PROCESSES_TOTAL) {
    if(wait_time_is_up() == -1) {
      fprintf(stderr, "oss is waiting to generate fork new child process\n");
      increment_clock();

      continue;
    }

    // before forking, check if current active processes < 18
    // IF >= 18, report this, increment the clock, and continue the loop
    if(resource_table->current_processes >= MAX_PROCESSES_RUNNING) {
      printf("oss: Warning: Max active processes reached. Skipping this round\n");

      increment_clock();
      continue;
    }
    if(resource_table->total_processes > MAX_PROCESSES_TOTAL) {
      printf("oss: Warning: Max total processes reached. Skipping this round\n");
      continue;
    }

    printf("It is time to fork a child!\n");

    resource_table->total_processes++;

    // increment 'current_processes' when forked, decrement it when child finishes
    pid_t child_pid = fork();

    if (child_pid == -1) {
      perror("oss: Error: Failed to fork a child process");
      cleanup();
      return -1;
    }

    if (child_pid == 0) {
      // attach memory again as child
      resource_table = (ResourceTable *)shmat(shmid, NULL, 0);

      int process_b = getRandom(500) + 1; // 1-500 for value of 'B'
      int b_length = snprintf( NULL, 0, "%d", process_b );
      char* process_b_str = malloc( b_length + 1 );
      snprintf( process_b_str, b_length + 1, "%d", process_b );

      int shmid_length = snprintf( NULL, 0, "%d", shmid );
      char* shmid_str = malloc( shmid_length + 1 );
      snprintf( shmid_str, shmid_length + 1, "%d", shmid );

      // add pid to resource process
      int pid = getNextPid();
      // busy wait until available pid to satisfy max running processes
      while(pid == -1) {
        sleep(1);
        printf("process needs to wait for available pid\n");
        pid = getNextPid();
      }
      printf("next pid got: %d\n", pid);
      resource_table->processes[pid].pid = pid;

      int pid_length = snprintf( NULL, 0, "%d", shmid );
      char* pid_str = malloc( pid_length + 1 );
      snprintf( pid_str, pid_length + 1, "%d", pid);

      // execl
      execl("./user", "./user", process_b_str, shmid_str, pid_str, (char *) NULL); // 1 arg: pass shmid
      perror("oss: Error: Child failed to execl");
      cleanup();
      exit(0);
    }
    else {
      // if it has the resources available AND if it is safe
        // check resource descriptor


      // can write message to logfile

      // pid_t wpid = wait(NULL);
      // if(wpid == -1) {
      //   perror("oss: Error: Failed to wait for child");
      //   cleanup();
      //   return 1;
      // }

      // fprintf(stderr, "A child has finished\n");
      // resource_table->current_processes--;
      

      // parent waits inside loop for child to finish
      int status;
      pid_t wpid = waitpid(child_pid, &status, WNOHANG);
      // fprintf(stderr, "\nwpid: %d\n", wpid);
      // fprintf(stderr, "status: %d\n\n", status);
      if (wpid == -1) {
        perror("oss: Error: Failed to wait for child");
        cleanup();
        return 1;
      }
      else if(wpid == 0) {
        // child is still running. in parent
        fprintf(stderr, "child is still running\n");

        // setup to receive messages
        resource_table->current_processes++;
        // resource_table->total_processes++;

        int is_terminate = 0;
        while(is_terminate == 0) {
          printf("\n(oss): Is beginning of terminate loop!\n");
          mymsg_t mymsg; // for queue
          
          // setup message receiver
          int msg_size = msgrcv(resource_table->queueid, &mymsg, MAX_MSG_SIZE, 0, 0);
          if(msg_size == -1) {
            perror("oss: Error: Could not receive message from child\n");
            cleanup();
            return 1;
          }

          fprintf(stderr, "after rcv. msg: %s\n", mymsg.mtext);

          // Parse Message
          int request_process;
          int request_type; // 0 is for consume, 1 is for release
          int resource_index;
          int resource_value;
          int resource_time_sec;
          int resource_time_ns;

          char *request_process_str = strtok(mymsg.mtext, "-");
          char *resource_type_str = strtok(NULL, "-");

          printf("req pid: %s, req type: %s\n", request_process_str, resource_type_str);

          request_process = atoi(request_process_str);
          request_type = atoi(resource_type_str);

          printf("request pid: %d\n", request_process);
          printf("resource type: %d\n", request_type);

          // log info to file
          char results_msg[MAX_MSG_SIZE];
          if(request_type == 1) { // is a request
            fprintf(stderr, "\nReceived Request\n");
            
            char *resource_index_str = strtok(NULL, "-");
            char *resource_value_str = strtok(NULL, "-");
            char *resource_time_sec_str = strtok(NULL, "-");
            char *resource_time_ns_str = strtok(NULL, "-");

            resource_index = atoi(resource_index_str);
            resource_value = atoi(resource_value_str);
            resource_time_sec = atoi(resource_time_sec_str);
            resource_time_ns = atoi(resource_time_ns_str);

            // print index and value
            // printf("resource index: %d\n", resource_index);
            // printf("resource value: %d\n", resource_value);
            // printf("resource time: %d sec, %d ns\n", resource_time_sec, resource_time_ns);

            add_time_to_clock(resource_time_sec, resource_time_ns);

            snprintf(results_msg, sizeof(results_msg), "Master has detected Process %d requesting R%d at time: %d:%d", request_process, resource_index, resource_time_sec, resource_time_ns);
            logmsg(results_msg);

            // check if resource is available by running deadlock detection
            printf("running deadlock detection...\n");


            char results_msg_deadlock[MAX_MSG_SIZE];
            snprintf(results_msg_deadlock, sizeof(results_msg_deadlock), "Master running deadlock detection at time %d:%d", resource_table->clock.sec, resource_table->clock.ns);
            logmsg(results_msg_deadlock);

            int is_unsafe = 0; // mocking result for now

            // log if unsuccessful state change
            if(is_unsafe == 1) {
              // TODO: get real data, replace the mocks with it
              logmsg("\tP1, P2, P3 deadlocked");
              logmsg("\tUnsafe state after granting request; request not granted");
              logmsg("\tP1 added to wait queue, waiting for R4");

              // wait until ready, then write message back to child
              int msg_type = 4;
              int deadlock_result = 2; // blocked
              char *buf_res;
              asprintf(&buf_res, "%d-%d-", deadlock_result, msg_type);

              // write message back to child
              fprintf(stderr, "oss: Writing Message to User\n");
              if(msgwrite(buf_res, MAX_MSG_SIZE, msg_type, resource_table->queueid) == -1) {
                perror("oss: Error: Could not write message back to child");
                return 1;
              }
            }
            // log results if safe
            else {
              char results_msg_3[MAX_MSG_SIZE];
              // log that it has been released if successful
              logmsg("\tSafe state after granting request");
              snprintf(results_msg_3, sizeof(results_msg_3), "\tMaster granting Process %d resource R%d at time %d:%d", request_process, resource_index, resource_table->clock.sec, resource_table->clock.ns);
              logmsg(results_msg_3);

              // increase # of granted requests
              resource_table->granted_requests++;
              if(resource_table->granted_requests % 20 == 0) {
                print_current_resources();
              }

              int msg_type = 4;
              int deadlock_result = 1; // 1 is approved, 2 is blocked
              char *buf_res;
              asprintf(&buf_res, "%d-%d-", deadlock_result, msg_type);

              // write message back to child
              fprintf(stderr, "oss: Writing Message to User\n");
              if(msgwrite(buf_res, MAX_MSG_SIZE, msg_type, resource_table->queueid) == -1) {
                perror("oss: Error: Could not write message back to child");
                return 1;
              }
            }
          }
          else if(request_type == 2) {
            fprintf(stderr, "\nReceived Release\n");
            char *resource_index_str = strtok(NULL, "-");
            char *resource_value_str = strtok(NULL, "-");
            char *resource_time_sec_str = strtok(NULL, "-");
            char *resource_time_ns_str = strtok(NULL, "-");

            resource_index = atoi(resource_index_str);
            resource_value = atoi(resource_value_str);
            resource_time_sec = atoi(resource_time_sec_str);
            resource_time_ns = atoi(resource_time_ns_str);

            // print index and value
            // printf("resource index: %d\n", resource_index);
            // printf("resource value: %d\n", resource_value);
            // printf("resource time: %d sec, %d ns\n", resource_time_sec, resource_time_ns);

            add_time_to_clock(resource_time_sec, resource_time_ns);

            snprintf(results_msg, sizeof(results_msg), "Master has acknowledged P%d releasing R%d at time: %d:%d", request_process, resource_index, resource_time_sec, resource_time_ns);
            logmsg(results_msg);

            // release the resource
            release_process_resource(request_process, resource_index, resource_value);

            // log that it has been released
            char results_msg_2[MAX_MSG_SIZE];
            snprintf(results_msg_2, sizeof(results_msg_2), "Master has released R%d at time: %d:%d", resource_index, resource_time_sec, resource_time_ns);
            logmsg(results_msg_2);

            int msg_type = 5;
            char *buf_res;
            asprintf(&buf_res, "%d-%d-Resources Released-", request_process, msg_type);

            // write message back to child
            fprintf(stderr, "oss: Writing Message to User\n");
            if(msgwrite(buf_res, MAX_MSG_SIZE, msg_type, resource_table->queueid) == -1) {
              perror("oss: Error: Could not write message back to child");
              return 1;
            }
          }
          else if(request_type == 3) {
            is_terminate = 1;

            char *resource_time_sec_str = strtok(NULL, "-");
            char *resource_time_ns_str = strtok(NULL, "-");

            resource_time_sec = atoi(resource_time_sec_str);
            resource_time_ns = atoi(resource_time_ns_str);

            add_time_to_clock(resource_time_sec, resource_time_ns);

            // print index and value
            // printf("resource time: %d sec, %d ns\n", resource_time_sec, resource_time_ns);

            fprintf(stderr, "\nReceived Termination\n");
            // is terminating. release all resources, then reset resource_table
            char results_msg_terminate[MAX_MSG_SIZE];
            snprintf(results_msg_terminate, sizeof(results_msg_terminate), "P%d terminated at time %d:%d", request_process, resource_time_sec, resource_time_ns);
            logmsg(results_msg_terminate);
            
            // log which resources freed
            char results_msg_terminate_release[MAX_MSG_SIZE];
            snprintf(results_msg_terminate_release, sizeof(results_msg_terminate_release), "\tResources Released: ");
            for(int res_index=0; res_index<RESOURCES_DEFAULT; res_index++) {
              // if the index 'res_index' is found in the array of indexes in the process's resource array, then log it, then clear it
              if(found_in_resource_array(res_index, request_process) == 1) {
                // log it
                snprintf(results_msg_terminate_release, sizeof(results_msg_terminate_release), "R%d:%d, ", resource_table->processes[request_process].resources[res_index].index, resource_table->processes[request_process].resources[res_index].allocation);
              }
            }

            logmsg(results_msg_terminate_release);

            // free pid
            freePid(request_process);
            release_process(request_process); // its pid
            // resource_table->total_processes++;
          }
        }
      }
      else {
        // child has finished
        fprintf(stderr, "A child has finished\n");
        resource_table->current_processes--; // when the process as finished
      }
    }

    printf("new # total processes: %d, new # active processes: %d\n", resource_table->total_processes, resource_table->current_processes);

    time_diff = increment_clock();
    sleep(1); // to debug
  }

  // Wait for all children to finish, after the main loop is complete
  while(wait(NULL) > 0) {
    printf("oss: Info: Waiting for all children to finish...\n");
  }
  
  // generate report and log to file
  generate_report();

  // cleanup
  cleanup();

  return 0;
}


void cleanup() {
  // message queue
  if(remmsgqueue(resource_table->queueid) == -1) {
    perror("oss: Error: Failed to remove message queue");
  }
  else printf("success remove msgqueue\n");
  // semaphore
  if(removesem(semid) == -1) {
    perror("runsim: Error: Failed to remove semaphore");
  }
  else printf("sucess remove sem\n");
  // shared memory
  if(detachandremove(shmid, resource_table) == -1) {
    perror("oss: Error: Failure to detach and remove memory");
  }
  else printf("success detatch\n");
}

// From textbook
int detachandremove(int shmid, void *shmaddr) {
  int error = 0;

  if (shmdt(shmaddr) == -1) {
    fprintf(stderr, "oss: Error: Can't detach memory\n");
    error = errno;
  }
  
  if ((shmctl(shmid, IPC_RMID, NULL) == -1) && !error) {
    fprintf(stderr, "oss: Error: Can't remove shared memory\n");
    error = errno;
  }

  if (!error)
    return 0;

  errno = error;

  return -1;
}

void logmsg(const char *msg) {
  FILE *fp = fopen(logfile, "a+");
  if(fp == NULL) {
    perror("oss: Error: Could not use log file.\n");
    return;
  }

  // Check if lines in the file >100,000
  if(check_line_count(fp) == -1) {
    fprintf(stderr, "line count reached");
    fclose(fp);
    return;
  }

  fprintf(fp, "%s\n", msg);
  fclose(fp);
}

void generate_report() {
  printf("working on generating report still\n");

  FILE *fp = fopen(logfile, "a+");
  if(fp == NULL) {
    perror("oss: Error: Could not open log file");
    return;
  }

  if(check_line_count(fp) == -1) {
    fprintf(stderr, "line count reached");
    fclose(fp);
    return;
  }

  // Actually gather report items for writing to file:
  // - how many requests have been granted immediately
  // - how many requests are granted after waiting for a bit
  // - number of processes terminated by deadlock algorithm
  // - number of processes terminated successfully and naturally (no deadlock)
  // - how many times the deadlock detection is run
  // - how many processes it had to terminate
  // - percentage of processes that got caught in a deadlock and had to be terminated, on average

  // --> mock for now
  fprintf(fp, "\nGenerating report info...\n");

  // Fill in with the real report info


  fclose(fp);
  return;
}

void print_current_resources() {
  // prints a table to logfile showing the current # of resources allocated to each process
  // Example:
  //    R0  R1  R3  R4  ...
  // P0 2   1   3   4   ...
  // P1 0   1   1   0   ...
  // P2 3   0   2   2   ...

  // open logfile
  FILE *fp = fopen(logfile, "a+");
  if(fp == NULL) {
    perror("oss: Error: Coudl not open log file");
    fclose(fp);
    return;
  }

  // check line count
  if(check_line_count(fp) == -1) {
    fprintf(stderr, "line count reached");
    fclose(fp);
    return;
  }

  // get information to log
  // --> mocking for now
  fprintf(fp, "Current System Resources:\n");
  
  // print headers
  for(int i=0; i<RESOURCES_DEFAULT; i++) {
    fprintf(fp, "\tR%d", i);
  }

  // print processes and their values for the resource
  for(int i=0; i<MAX_PROCESSES_RUNNING; i++) {
    fprintf(fp, "P%d", i);

    Process found_process = get_process_by_pid(i);

    // print each of the process's resource allocations
    for(int res_index=0; res_index<RESOURCES_DEFAULT; res_index++) {
      if(found_process.resources[res_index].index == -1)
        fprintf(fp, "\t%d", 0);
      else
        fprintf(fp, "\t%d", found_process.resources[res_index].allocation);
    }
  }

  fclose(fp);
  return;
}

int check_line_count(FILE *fp) {
  int linecount = 0;
  char c;
  while(1) {
    if(feof(fp))
      break;
    c = fgetc(fp);
    if(c == '\n')
      linecount++;
  }

  if(linecount > LOGFILE_MAX_LINES) {
    perror("oss: Error: logfile has exceeded max lines");
    return -1;
  }

  return 0;
}

int wait_time_is_up() {
  // implement as critical section
  // compare next_sec and next_ns with what's in the process table
  if(next_fork.sec < resource_table->clock.sec) {
    return 0;
  }
  if(next_fork.sec == resource_table->clock.sec) {
    if(next_fork.ns < resource_table->clock.ns) {
      return 0;
    }
  }

  return -1; // -1 means not
}

void generate_next_child_fork() {
  int random_ms = getRandom(500) + 1; // 1-500 milliseconds
  int ns = random_ms * MS_NS_CONVERSION;
  // set next__fork to current clock
  next_fork.sec = resource_table->clock.sec;
  next_fork.ns = resource_table->clock.ns;
  if((next_fork.ns + ns) > NANOSECONDS) {
    int remainder_ns = next_fork.ns + ns;
    next_fork.sec++;
    next_fork.ns = remainder_ns;
  }
  else
    next_fork.ns = next_fork.ns + ns;
}

Clock increment_clock() {
  // increment_clock_round but as a critical section
  wait_sem(semid, semwait, 1);
  Clock time_temp = increment_clock_round();
  signal_sem(semid, semsignal, 1);
  
  return time_temp;
}

// xxx:xxxx
// char * get_time_string() {  }

void init_pids() {
  for(int i=0; i<MAX_PROCESSES_RUNNING; i++) {
    available_pids[i] = -1;
  }
}

int getNextPid() {
  for(int i=0; i<MAX_PROCESSES_RUNNING; i++) {
    if(available_pids[i] == -1) {
      available_pids[i] = i;
      return i;
    }
  }
  return -1;
}

void freePid(int pid) {
  for(int i=0; i<MAX_PROCESSES_RUNNING; i++) {
    if(available_pids[i] == pid) {
      available_pids[i] = -1;
      return;
    }
  }
}

int found_in_resource_array(int resource_index, int process_index) {
  // search resource array at process_index for the value. return 1 if found, 0 if not
  for(int b=0; b<RESOURCES_DEFAULT; b++) {
    if(resource_table->processes[process_index].resources[b].index == resource_index) {
      // it exists, so find the amount allocated and log it
      if(resource_table->processes[process_index].resources[b].allocation > 0) {
        printf("res %d had %d allocated to process %d\n", resource_index, resource_table->processes[process_index].resources[b].allocation, process_index);
        return 1;
      }
    }
  }

  return 0;
}
