#ifndef RESOURCE_TABLE_H
#define RESOURCE_TABLE_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "config.h"
#include "utils.h"

typedef struct {
  const char *name;
  int shareable;
  int n_resources;

  // to manage activities that affect the resources
  int request;
  int allocation;
  int release;
} Resource; // ResourceDescriptor

typedef struct {
  int queueid;
  int total_processes;
  int current_processes;
  Clock clock;
  Resource resources[20];
} ResourceTable;

int initialize_resource_table();
Resource init_resource();
void print_resources();

Clock increment_clock_round();
Clock add_time_to_clock(int sec, int ns);


#endif