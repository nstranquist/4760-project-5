#include "deadlock_detection.h"

/**
 * Deadlock Detection and Resolution in Project 5: Resource Management
 * 
 * - Use the deadlock avoidance strategy, using maximum claims, to manage resources.
 * 
 */

// NOTE: in 'bool' types, 1 is true, 0 is false

// sending everything as constant. Should not be changed during within this function
// - req is the request vector (see example)
// - avail is the availability vector (see example)
// - pnum is the process number
// - num_res is the number of resources being requested
bool request_is_less_than_available(const int *req, const int *avail, const int pnum, const int num_res) {
  // req is a 1-dimensional array, so for resource A, B, and C, would be i.e [0, 2, 1]
  int i;
  // if none of the requests is more than what is available, then terminate the loop normally, where i is # of resources
  for(i = 0; i < num_res; i++) {
    // if the loop breaks, i will be less than number of resources, so this will return false, meaning the request will not be less than available
    if(req[pnum * num_res + i] > avail[i]) {
      break;
    }
  }

  return (i == num_res);
}


bool deadlock(const int *available, const int m, const int n, const int *request, const int *allocated) {
  int work[m]; // m resources
  bool finish[n]; // n processes (bool)

  // copy availability vector into work vector for each resource
  for(int i=0; i < m; work[i] = available[i++]);
  // for each process, initialize each finish element to false
  for(int i=0; i < n; finish[i++] = false);

  // for each process:
  // - IF process is finished, skip/continue
  // - IF process request is less than available (and it isn't finished), set finished[p] to true and delete all the errors
  // - ELSE skip and go to the next process
  int p;
  for(p = 0; p < n; p++) {
    if(finish[p]) continue;
    if(request_is_less_than_available(request, work, p, m)) {
      // set p to finished
      finish[p] = true;
      // delete all arrows/errors, updating the work vector
      for(int i=0; i < m; i++) {
        work[i] += allocated[p*m+i];
      }
      // reinitialize p to -1
      p = -1;
    }
  }

  // 
  for(p = 0; p < n; p++) {
    // if process p is not finished, break and it will be less than n (so returns true)
    if(!finish[p])
      break;
  }

  // will return true if p != n, which indicates a deadlock
  return (p != n);
}