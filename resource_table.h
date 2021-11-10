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
  int total_processes;
  Clock clock;
  Resource resources[20];
} ResourceTable;

int initialize_resource_table();
Resource init_resource();
void print_resources();


#endif