#ifndef MONITORPROCESSCOMMANDS_H
#define MONITORPROCESSCOMMANDS_H

#include "requests.h"

void checkSkiplist(hashMap *, char *, char *, int, int, int, requests *);
void prematureExit(char **, char, requests *);
#endif