#ifndef QUEUE_H
#define QUEUE_H

#include "config.h"

#define MAX_MSG_SIZE 4096

typedef struct mymsg_t {
  // message defaults
  long mtype;
  char mtext[MAX_MSG_SIZE];
} mymsg_t;

int remmsgqueue(int queueid);
int msgwrite(void *buf, int len, int msg_type, int queueid);
int msgprintf(char *fmt, int type, int queueid, ...);
int initqueue(int key);
void print_message(mymsg_t msg);


#endif