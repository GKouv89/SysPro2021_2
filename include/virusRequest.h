#ifndef VIRUSREQUEST_H
#define VIRUSREQUEST_H

#include "requests.h"

typedef struct virreq{
    char *virusName;
    namedRequests **requests;
    int capacity;
    int length;
}virusRequest;

void create_virusRequest(virusRequest **, char *);
int isEqual_virusRequest(virusRequest *, char *);
void resize_virusRequest(virusRequest *);
namedRequests* addRequest(virusRequest *, char *, char *);
namedRequests* findRequest(virusRequest *, char *, char *);
void gatherStatistics(virusRequest *, char *, char *, char *, int, requests *);
void destroy_virusRequest(virusRequest **);
#endif
