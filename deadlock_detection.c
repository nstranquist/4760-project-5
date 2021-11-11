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

/**
 * Banker's Algorithm - Data Structures
 * - available: vector of length n, # available resources of each type
 *    - available[j] = k, where k instances are of resource class Rj available
 * - maximum: n x m matrix that defines maximum demand for each process
 *    - maximum[i, j] = k, where process pi may request at most k instances of resource class Rj
 * - allocation: n x m matrix that defines the # resources of each type currently allocated to each process
 *    - allocation[i, j] = k, where process pi is currently allocated to k instances of resource class Rj
 * - need: n x m matrix that indicates the remaining resource needs of each process
 *    - need[i, j] = k, where process pi may need k more instances of resource type Rj in order to complete its task
 *    - need[i, j] = maximum[i, j] - allocation[i, j]
 */
void bankers_algorithm(const int *available, const int *request, const int *allocated, const int *need, const int i, const int *p) {
  if(request[i] > need[i]) {
    throw("Asked more than initial request");
  }

  if(request[i] <= available) {
    available -= request[i];
    allocation[i] += request[i];
    need[i] -= request[i];
  }
  else {
    p[i].wait(); // send process i into wait state
  }
}