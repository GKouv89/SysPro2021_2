#ifndef SETOFBFS_H
#define SETOFBFS_H

#include "bloomfilter.h"

typedef struct setofbfs{
    char *virusName;
    // An array of bloomfilters on virusName, used for lookup.
    // Each index corresponds to a monitor, a.k.a.
    // the virusBF[0] contains the filter for the countries
    // handled by monitorProcess 0, and so forth.
    int capacity;
    int length;
    int sizeOfBloom;
    bloomFilter **bfs;
} setofbloomfilters;

void create_setOfBFs(setofbloomfilters **, char *, int, int);
int isEqual_setOfBFs(setofbloomfilters *, unsigned char *);
void add_BFtoSet(setofbloomfilters *set, int);
int lookup_bf_vaccination(setofbloomfilters *, int, unsigned char *);
void read_BF(setofbloomfilters *, int, int, int, int);
void destroy_setOfBFs(setofbloomfilters **);
#endif