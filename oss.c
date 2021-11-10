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
#include <sys/stat.h>
#include <getopt.h>
#include <math.h> // for randomness
#include "config.h"
#include "resource_table.h"
#include "semaphore_manager.h"
#include "utils.h"

#define PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)


// global variables
extern ResourceTable *resource_table;
const char* logfile = "oss.log";
int shmid; // to manage shared memory
int semid; // to manage semaphore
struct sembuf semsignal[1];
struct sembuf semwait[1];


// function definitions
int detachandremove(int shmid, void *shmaddr);
void logmsg(const char *msg);
void cleanup();

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

  // generateReport();

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
  shmid = shmget(IPC_PRIVATE, sizeof(ResourceTable), IPC_CREAT | 0666); // (struct ProcessTable)
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

  // Create semaphore containing a single element
  if((semid = semget(IPC_PRIVATE, 1, PERMS)) == -1) {
    perror("runsim: Error: Failed to create private semaphore\n");
    return 1;
  }

  setsembuf(semwait, 0, -1, 0); // decrement first element of semwait
  setsembuf(semsignal, 0, 1, 0); // increment first element of semsignal



  // Start program timer
  alarm(MAX_SECONDS);

  
  // init clock
  resource_table->clock.sec = 0;
  resource_table->clock.ns = 0;

  // init resource table
  initialize_resource_table();

  print_resources();


  sleep(10);


  // start process loop (main logic)
  while(resource_table->total_processes < MAX_PROCESSES_TOTAL) {
    sleep(1);    

    resource_table->total_processes++;
  }
  

  // cleanup
  cleanup();

  return 0;
}


void cleanup() {
  if(removesem(semid) == -1) {
    perror("runsim: Error: Failed to remove semaphore\n");
  }
  else printf("sucess remove sem\n");
  if(detachandremove(shmid, resource_table) == -1) {
    perror("oss: Error: Failure to detach and remove memory\n");
  }
  else printf("success detatch\n");
  // semaphore
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

  // Check if lines in the file >10,000
  int linecount = 0;
  char c;
  while(1) {
    if(feof(fp))
      break;
    c = fgetc(fp);
    if(c == '\n')
      linecount++;
  }
}
