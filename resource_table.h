#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "config.h"

typedef struct {
  const char *name;
} Resource;

typedef struct {
  Clock *clock;
  Resource *resource;
} ResourceTable;

void printTable();