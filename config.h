#ifndef CONFIG_H
#define CONFIG_H

#define MAX_SECONDS 5

#define MAX_PROCESSES 40

typedef struct {
  int sec;
  int ns;
} Clock;

#endif