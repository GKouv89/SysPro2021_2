#include "acutest.h"
#include "../include/virusRequest.h"

void test_virusRequest_creation_deletion(){
    virusRequest *vr;
    create_virusRequest(&vr , "SARS-1");
    TEST_ASSERT(vr->requests != NULL);
    destroy_virusRequest(&vr);
}

void test_sameCountryDifferentDate(){
    virusRequest *vr;
    create_virusRequest(&vr, "SARS-1");
    char *countryTo = "GREECE";
    char *date1 = "01-01-1998";
    char *date2 = "30-08-1999";
    addRequest(vr, countryTo, date1);
    addRequest(vr, countryTo, date2);
    namedRequests *req1 = findRequest(vr, countryTo, date1);
    acceptNamedRequest(req1);
    namedRequests *req2 = findRequest(vr, countryTo, date2);
    rejectNamedRequest(req2);
    TEST_ASSERT(vr->requests[0]->totalOnThisDate == 1);
    TEST_ASSERT(vr->requests[1]->totalOnThisDate == 1);
    TEST_ASSERT(vr->requests[0]->acceptedOnThisDate == 1);
    TEST_ASSERT(vr->requests[1]->rejectedOnThisDate == 1);
    destroy_virusRequest(&vr);
}   

void test_sameCountrySameDate(){
    virusRequest *vr;
    create_virusRequest(&vr, "SARS-1");
    char *countryTo = "GREECE";
    char *date = "01-01-1998";
    addRequest(vr, countryTo, date);
    namedRequests *req = findRequest(vr, countryTo, date);
    acceptNamedRequest(req);
    rejectNamedRequest(req);
    TEST_ASSERT(vr->length == 1);
    TEST_ASSERT(vr->requests[0]->totalOnThisDate == 2);
    TEST_ASSERT(vr->requests[0]->acceptedOnThisDate == 1);
    TEST_ASSERT(vr->requests[0]->rejectedOnThisDate == 1);
    destroy_virusRequest(&vr);
}


TEST_LIST = {
  {"creation_deletion", test_virusRequest_creation_deletion},
  {"same_country_different_date", test_sameCountryDifferentDate},
  {"same_country_same_date", test_sameCountrySameDate},
  {NULL, NULL}
};