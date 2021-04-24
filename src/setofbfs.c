#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/setofbfs.h"

void create_setOfBFs(setofbloomfilters **set, char *virusName, int arrayCount, int sizeOfBloom){
    (*set) = malloc(sizeof(setofbloomfilters));
    (*set)->capacity = arrayCount;
    (*set)->length = 0;
    (*set)->sizeOfBloom = sizeOfBloom;
    (*set)->bfs = malloc(arrayCount*sizeof(bloomFilter *));
    strcpy((*set)->virusName, virusName);
}

int isEqual_setOfBFs(setofbloomfilters *set, unsigned char *virusName){
    if(strcmp(set->virusName, virusName) == 0){
        return 1;
    }
    return 0;
}

void add_BFtoSet(setofbloomfilters *set){
    create_bloomFilter(&(set->bfs[set->length]), set->sizeOfBloom, 16);
    (set->length)++;
}

void destroy_setOfBFs(setofbloomfilters **set){
    for(int i = 0; i < (*set)->length; i++){
        destroy_bloomFilter(&((*set)->bfs[i]));
    }
    free((*set)->bfs);
    free((*set)->virusName);
}