#ifndef MONITORPROCESSCOMMANDS_H
#define MONITORPROCESSCOMMANDS_H

#include "requests.h"

void checkSkiplist(hashMap *, char *, char *, int, int, int, requests *);
void prematureExit(int, int, char **, int, requests *);
void checkVacc(hashMap *, hashMap *, char *, int, int, int);
#endif