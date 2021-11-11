#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "config.h"
#include "utils.h"


int main(int argc, char *argv[]) {
  printf("In user!");

  if(argc != 2) {
    perror("user: Usage: ./user b\n");
    return 1;
  }
  char *b_str = argv[1];
  printf("b from params: %s\n", b_str);

  if(!atoi(b_str)) {
    perror("user: Error: paramter received for 'B' is not a number\n");
    return 1;
  }

  int b = atoi(b_str);

  // generate random from [0, B], where B is the upper bound for when a process should request a new resource
  int next_request = getRandom(b + 1);
  printf("next request: %d\n", next_request);


  return 0;
}
