#ifndef DEADLOCK_DETECTION_H
#define DEADLOCK_DETECTION_H

#include <stdbool.h>
#include <stdlib.h>

bool request_is_less_than_available(const int *req, const int *avail, const int pnum, const int num_res);

bool deadlock(const int *available, const int m, const int n, const int *request, const int *allocated);

void bankers_algorithm(const int *available, const int *request, int *allocated, int *need, const int i, int *p);


#endif