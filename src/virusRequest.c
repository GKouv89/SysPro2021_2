#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../include/virusRequest.h"

void create_virusRequest(virusRequest **vr, char *virusName){
    *vr = malloc(sizeof(virusRequest));
    (*vr)->virusName = malloc((strlen(virusName) + 1)*sizeof(char));
    strcpy((*vr)->virusName, virusName);
    (*vr)->capacity = 10;
    (*vr)->requests = malloc((*vr)->capacity*sizeof(namedRequests *));
    (*vr)->length = 0;
}

int isEqual_virusRequest(virusRequest *vr, char *str){
    if(strcmp(vr->virusName, str) == 0){
        return 1;
    }
    return 0;
}

void resize_virusRequest(virusRequest *vr){
    vr->capacity *= 2;
    namedRequests **temp = realloc(vr->requests, vr->capacity*sizeof(namedRequests *));
    assert(temp != NULL);
    vr->requests = temp;
}

namedRequests* addRequest(virusRequest *vr, char *countryTo, char *date){
    namedRequests *nreq;
    if((nreq = findRequest(vr, countryTo, date)) != NULL)
        return nreq;
    namedRequests *new;
    create_namedRequest(&new, countryTo, date);
    vr->requests[vr->length] = new;
    vr->length++;
    if(vr->length == vr->capacity){
        resize_virusRequest(vr);
    }
    return new;
}

namedRequests* findRequest(virusRequest *vr, char *countryTo, char *date){
    for(int i = 0; i < vr->length; i++){
        if(strcmp(vr->requests[i]->countryTo, countryTo) == 0 && strcmp(vr->requests[i]->date, date) == 0){
            return vr->requests[i];
        }
    }
    return NULL;
}

void destroy_virusRequest(virusRequest **vr){
    free((*vr)->virusName);
    for(int i = 0; i < (*vr)->length; i++){
        destroy_namedRequest(&((*vr)->requests[i]));
    }
    free((*vr)->requests);
    free(*vr);
}

