#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "config.h"
#include "utils.h"


int main(int argc, char *argv[]) {
  printf("In user!");

  // generate random from [0, B], where B is the upper bound for when a process should request a new resource
  int temp_b = 5;
  int next_request = getRandom(temp_b + 1);
  printf("next request: %d\n", next_request);


  return 0;
}
