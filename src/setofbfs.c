#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../include/setofbfs.h"

void create_setOfBFs(setofbloomfilters **set, char *virusName, int arrayCount, int sizeOfBloom){
    (*set) = malloc(sizeof(setofbloomfilters));
    (*set)->capacity = arrayCount;
    (*set)->length = 0;
    (*set)->sizeOfBloom = sizeOfBloom;
    (*set)->bfs = malloc(arrayCount*sizeof(bloomFilter *));
    for(int i = 0; i < arrayCount; i++){
        (*set)->bfs[i] = NULL;
    }
    (*set)->virusName = malloc((strlen(virusName) + 1)*sizeof(char));
    strcpy((*set)->virusName, virusName);
}

int isEqual_setOfBFs(setofbloomfilters *set, unsigned char *virusName){
    if(strcmp(set->virusName, virusName) == 0){
        return 1;
    }
    return 0;
}

void add_BFtoSet(setofbloomfilters *set, int index){
    // when select chooses a child to receive the filter from,
    // the index is the same as the index of the pipe's file descriptor
    // in the read and write file descriptor arrays of the parent.
    assert(set->bfs[index] == NULL);
    create_bloomFilter(&(set->bfs[index]), set->sizeOfBloom, 16);
    (set->length)++;
}

int lookup_bf_vaccination(setofbloomfilters *set, int index, unsigned char *str){
    return lookup_in_bloomFilter(set->bfs[index], str);
}

void destroy_setOfBFs(setofbloomfilters **set){
    for(int i = 0; i < (*set)->length; i++){
        destroy_bloomFilter(&((*set)->bfs[i]));
        (*set)->bfs[i] = NULL;
    }
    free((*set)->bfs);
    free((*set)->virusName);
    free(*set);
    *set = NULL;
}