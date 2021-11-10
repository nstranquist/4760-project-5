#include "utils.h"


int getRandom(int upper) {
  // get number from [0, upper)
  int randNum = rand() % upper;
  return randNum;
}

int getRandomRange(int lower, int upper) {
  // get number from lower to upper
  int randNum = (rand() % (upper - lower + 1)) + lower;
  return randNum;
}

