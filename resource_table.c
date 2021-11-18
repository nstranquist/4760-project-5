#include "resource_table.h"


ResourceTable *resource_table;

const char alphabet[26][10] = {
  "ResA",
  "ResB",
  "ResC",
  "ResD",
  "ResE",
  "ResF",
  "ResG",
  "ResH",
  "ResI",
  "ResJ",
  "ResK",
  "ResL",
  "ResM",
  "ResN",
  "ResO",
  "ResP",
  "ResQ",
  "ResR",
  "ResS",
  "ResT",
  "ResU",
  "ResV",
  "ResW",
  "ResX",
  "ResY",
  "ResZ",
};

int initialize_resource_table() {
  // random between 15-25% should be shareable resources
  int amountShareable = getRandomRange(15, 25);
  printf("amount shareable: %d\n", amountShareable);

  int realAmountShareable = amountShareable / 5;
  printf("real amount shareable: %d\n", realAmountShareable);

  resource_table->total_processes = 0;
  resource_table->total_resources = RESOURCES_DEFAULT;

  // initialize all 20 resource descriptors
  for(int i=0; i<RESOURCES_DEFAULT; i++) {
    int shareable = 0;
    if(i<= (realAmountShareable - 1))
      shareable = 1;
    int n_resources = getRandom(10) + 1;
    resource_table->resources[i] = init_resource(shareable, n_resources, alphabet[i]);
  }

  // initialize all 20 of the process helpers
  for(int i=0; i<RESOURCES_DEFAULT; i++) {
    resource_table->processes[i] = init_process();
  }
}

Resource init_resource(int shareable, int n_resources, const char *name) {
  Resource new;

  new.name = name;
  new.shareable = shareable; // where 0 is not shareable, and 1 is shareable
  new.n_resources = n_resources; // 1-10

  // defaults for now
  new.allocation = 0;
  new.release = 0;
  new.request = 0;

  new.available = 0; // todo: generate number between 1-10

  return new;
}

Process init_process() {
  Process new_process;

  new_process.pid = -1;

  for(int j=0; j<RESOURCES_DEFAULT; j++) {
    new_process.resources[j].index = -1; // -1 means no index assigned
    new_process.resources[j].allocation = 0;
  }

  return new_process;
}

// void print_current_resources() {
//   printf("\nPrinting Current System Resources to logfile... %d current processes\n\n", resource_table->current_processes);
  
  
//   // now, also print to logfile
//   FILE *logfile = fopen("oss.log", "a+");
//   if(logfile == NULL) {
//     perror("oss: Error: could not open logfile");
//     return;
//   }

//   fprintf(logfile, "\nCurrent System Resources:\n");

//   // print all resource names in header row
//   for(int i=0; i<RESOURCES_DEFAULT; i++) {
//     fprintf(logfile, "\tR%d", i);
//   }
//   fprintf(logfile, "\n");

//   // print the rows of the resources for each process
//   for(int i=0; i<resource_table->current_processes; i++) {
//     fprintf(logfile, "\nP%d", i);

//     // for each process, print its resources allocated
//     for(int j=0; j<RESOURCES_DEFAULT; j++) {
//       fprintf(logfile, "\t%d", resource_table->processes[i].resources[j].allocation);
//     }
//   }
//   fprintf(logfile, "\n");

//   fclose(logfile);
// }

// function implementations to work with process table
Clock increment_clock_round() {
  // get random ns [0,1000] (ms)
  int ms = getRandom(MILISECONDS+1);

  // convert ms to ns
  int ns = ms * 1000000;

  // create new time with 1 + ns
  Clock time_diff = add_time_to_clock(1, ns);

  return time_diff;
}

Clock add_time_to_clock(int sec, int ns) {
  // add seconds
  resource_table->clock.sec = resource_table->clock.sec + sec;

  // check ns for overflow, handle accordingly
  if((resource_table->clock.ns + ns) >= NANOSECONDS) {
    int remaining_ns = (resource_table->clock.ns + ns) - NANOSECONDS;
    resource_table->clock.sec += 1;
    resource_table->clock.ns = remaining_ns;
  }
  else
    resource_table->clock.ns += ns;
  
  printf("\n");

  printf("new time: %d sec, %d ns\n", resource_table->clock.sec, resource_table->clock.ns);

  Clock time_diff;
  time_diff.sec = sec;
  time_diff.ns = ns;

  return time_diff;
}

int request(int index, int amount) {
  // mock for now
  int error = 0;

  // request system resources, update the resource table accordingly
  printf("requesting %d resources for descriptor #%d\n", amount, index);


  if(error == 1) {
    return -1;
  }
  return 0;
}

int allocate(int index, int amount) {
  // mock for now
  int error = 0;

  // allocate system resources, update the resource table accordingly
  printf("allocating %d resources for descriptor #%d\n", amount, index);
  

  if(error == 1) {
    return -1;
  }
  return 0;
}

int release(int index) {
  // mock for now
  int error = 0;

  // release system resources, update the resource table accordingly
  printf("releasing resources for descriptor #%d\n", index);

  if(error == 1) {
    return -1;
  }
  return 0;
}

Process get_process_by_pid(int pid) {
  for(int i=0; i<MAX_PROCESSES_RUNNING; i++) {
    if(resource_table->processes[i].pid == pid)
      return resource_table->processes[i];
  }
  printf("oss: Warning: No process matching the id %d was found\n", pid);
  return resource_table->processes[pid];
}

void release_process(int pid) {
  if(pid == -1) {
    fprintf(stderr, "oss: Warning: cannot reset process with pid of -1\n");
    return;
  }

  for(int i=0; i<MAX_PROCESSES_RUNNING; i++) {
    if(resource_table->processes[i].pid == pid) {
      Process new_process = init_process();
      resource_table->processes[i] = new_process;
    }
  }
}

void release_process_resource(int pid, int resource_index, int resource_allocation) {
  if(pid == -1) {
    fprintf(stderr, "oss: Warning: cannot reset process resource with pid of -1\n");
    return;
  }

  for(int i=0; i<MAX_PROCESSES_RUNNING; i++) {
    if(resource_table->processes[i].pid == pid) {
      int current_allocation = resource_table->processes[i].resources[resource_index].allocation;
      if(current_allocation - resource_allocation <= 0) {
        resource_table->processes[i].resources[resource_index].allocation = 0;
        resource_table->processes[i].resources[resource_index].index = -1;
      }
      else
        resource_table->processes[i].resources[resource_index].allocation -= resource_allocation;

      return;
    }
  }
}