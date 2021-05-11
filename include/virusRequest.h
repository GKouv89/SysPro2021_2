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
void resize_virusRequest(virusRequest *vr);
void addRequest(virusRequest *, char *, char *);
namedRequests* findRequest(virusRequest *, char *, char *);
void destroy_virusRequest(virusRequest **);
#endif
