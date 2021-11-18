#ifndef CONFIG_H
#define CONFIG_H

#define MAX_SECONDS 50 // 5

#define MAX_PROCESSES_TOTAL 1 // 40

#define MAX_PROCESSES_RUNNING 18

#define NANOSECONDS 1000000000
#define MILISECONDS 1000
#define MS_NS_CONVERSION 1000000

#define LOGFILE_MAX_LINES 100000

#define MAX_MSG_SIZE 4096

#define RESOURCES_DEFAULT 10

typedef struct {
  int sec;
  int ns;
} Clock;

#endif