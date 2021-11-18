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

void print_resources() {
  printf("\n");
  printf("#, name, shareable?, n_resources\n");
  for(int i=0; i<RESOURCES_DEFAULT; i++) {
    printf("Resource #%d: %s, %d, %d\n", i, resource_table->resources[i].name, resource_table->resources[i].shareable, resource_table->resources[i].n_resources);
  }
  printf("\n");
}

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

