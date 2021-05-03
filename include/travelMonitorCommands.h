#ifndef TRAVELMONITORCOMMANDS_H
#define TRAVELMONITORCOMMANDS_H

#include "requests.h"

void travelRequest(hashMap *, hashMap *, char *, char *, char *, char *, int, int *, int *, requests *);
int passCommandLineArgs(int, char *, int, int, char *);
void receiveBloomFiltersFromChild(hashMap *, int, int, int, int, int, int);

#endif
