#include "resource_table.h"


ResourceTable *resource_table;

int initialize_resource_table() {
  // random between 15-25% should be shareable resources
  int amountShareable = getRandomRange(15, 25);
  printf("amount shareable: %d\n", amountShareable);

  int realAmountShareable = amountShareable / 5;
  printf("real amount shareable: %d\n", realAmountShareable);

  resource_table->total_processes = 0;

  // initialize all 20 resource descriptors
  for(int i=0; i<20; i++) {
    int shareable = 0;
    if(i<= (realAmountShareable - 1))
      shareable = 1;
    int n_resources = getRandom(10) + 1;
    resource_table->resources[i] = init_resource(shareable, n_resources);
  }
}

Resource init_resource(int shareable, int n_resources) {
  Resource new;

  new.name = "hello";
  new.shareable = shareable; // where 0 is not shareable, and 1 is shareable
  new.n_resources = n_resources;

  // defaults for now
  new.allocation = 0;
  new.release = 0;
  new.request = 0;

  return new;
}

void print_resources() {
  printf("\n");
  printf("#, name, shareable?, n_resources\n");
  for(int i=0; i<20; i++) {
    printf("Resource #%d: %s, %d, %d\n", i, resource_table->resources[i].name, resource_table->resources[i].shareable, resource_table->resources[i].n_resources);
  }
  printf("\n");
}