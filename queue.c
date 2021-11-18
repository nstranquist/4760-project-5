#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include "queue.h"

#define PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)


int initqueue(int key) {
  int queueid = msgget(key, PERMS | IPC_CREAT);

  if (queueid == -1) {
    perror("os: Error: could not initialize queue\n");
    return -1;
  }

  return queueid;
}

// from textbook
int msgprintf(char *fmt, int msg_type, int queueid, ...) {               /* output a formatted message */
  // va_list ap;
  // char ch;
  // int error = 0;
  // int len;

  // mymsg_t *mymsg;

  // va_start(ap, fmt);                       /* set up the format for output */

  // len = vsnprintf(&ch, 1, fmt, ap);              /* how long would it be ? */

  // if ((mymsg = (mymsg_t *)malloc(sizeof(mymsg_t) + len)) == NULL) {
  //   perror("oss: Error: Could not allocate space for message\n");
  //   return -1;
  // }

  // vsprintf(mymsg->mtext, fmt, ap);                 /* copy into the buffer */

  // mymsg->mtype = msg_type; // type always 1 or 2 (CPU / IO)

  // if (msgsnd(queueid, mymsg, len + 1, 0) == -1) {
  //   perror("oss: Error: could not send message with msgsnd\n");
  //   error = errno;
  // }

  // free(mymsg);

  // if (error) {
  //   errno = error;
  //   return -1;
  // }

  return 0;
}

int msgwrite(void *buf, int len, int msg_type, int queueid) {     /* output buffer of specified length */
  int error = 0;
  mymsg_t *mymsg;

  if ((mymsg = (mymsg_t *)malloc(sizeof(mymsg_t) + len - 1)) == NULL) {
    perror("oss: Error: Could not allocate space for message");
    return -1;
  }

  memcpy(mymsg->mtext, buf, len);

  mymsg->mtype = msg_type; // 1 or 2
  if (msgsnd(queueid, mymsg, len, 0) == -1) {
    perror("oss: Error: Could not send the message");
    error = errno;
  }

  free(mymsg);

  if (error) {
    errno = error;
    return -1;
  }

  return 0;
}

int remmsgqueue(int queueid) {
  return msgctl(queueid, IPC_RMID, NULL);
}

void print_message(mymsg_t msg) {
  printf("-----------Printing Message: ...------------\n");
  printf("%ld, %s\n", msg.mtype, msg.mtext);
  // printf("resource: %d, amount: %d\n", msg.resource_index, msg.resource_request);
}