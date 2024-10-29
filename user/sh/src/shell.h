#pragma once

typedef struct cmd {
  char *name;
  void (*function)();
} cmd;