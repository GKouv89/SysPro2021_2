#ifndef TRAVELMONITORCOMMANDS_H
#define TRAVELMONITORCOMMANDS_H

#include "requests.h"

void travelRequest(hashMap *, hashMap *, char *, char *, char *, char *, int, int *, int *, requests *);
void addVaccinationRecords(hashMap *, hashMap *, char *, pid_t *, int *, int *, int, int, int);
void passCommandLineArgs(int, int, int, int, char *);
void receiveBloomFiltersFromChild(hashMap *, int, int, int, int, int, int);
void childReplacement(hashMap *, hashMap *, pid_t, int **, int *, int *, int, int, int, char *);
void noMoreCommands(struct sigaction *, int, pid_t *, hashMap *, requests *);
void searchVaccinationStatus(int *, int *, int , int , char *);
#endif
