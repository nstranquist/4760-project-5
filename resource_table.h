#ifndef RESOURCE_TABLE_H
#define RESOURCE_TABLE_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "config.h"
#include "utils.h"

// each will request 1 or 1-3 resources
typedef struct {
  const char *name;
  int shareable;
  int n_resources; // # resource instances

  // to manage activities that affect the resources
  int *request;
  int *allocation;
  int *release;
  int *available; // how many resources are available
} Resource; // ResourceDescriptor

typedef struct {
  int total_resources; // 10

  int queueid;
  int total_processes;
  int current_processes;

  Clock clock;
  Resource resources[RESOURCES_DEFAULT]; // todo: define # of resource descriptors in configuration
} ResourceTable;

int initialize_resource_table();
Resource init_resource(int shareable, int n_resources, const char *name);
void print_resources();

Clock increment_clock_round();
Clock add_time_to_clock(int sec, int ns);

// functions to manage the system resources
int request(int index, int amount);
int allocate(int index, int amount);
int release(int index);


#endif