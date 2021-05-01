#include "acutest.h"
#include "../include/dateOps.h"

void test_years_difference(){
    char *dateOfTravel = "03-04-2018";
    char *dateOfVaccination = "03-04-2016";
    TEST_ASSERT(dateDiff(dateOfTravel, dateOfVaccination) == 1);
}

void test_six_months_diff_year(){
    char *dateOfTravel = "03-04-2018";
    char *dateOfVaccination = "03-10-2017";
    TEST_ASSERT(dateDiff(dateOfTravel, dateOfVaccination) == 0);
}

void test_less_than_six_months_diff_year(){
    char *dateOfTravel = "03-04-2018";
    char *dateOfVaccination = "04-10-2017";
    TEST_ASSERT(dateDiff(dateOfTravel, dateOfVaccination) == 0);
}

void six_months_same_year(){
    char *dateOfTravel = "03-11-2018";
    char *dateOfVaccination = "02-05-2018";
    TEST_ASSERT(dateDiff(dateOfTravel, dateOfVaccination) == 1);
}

void more_than_six_months_same_year(){
    char *dateOfTravel = "03-11-2018";
    char *dateOfVaccination = "03-02-2018";
    TEST_ASSERT(dateDiff(dateOfTravel, dateOfVaccination) == 1);
}

void exactly_six_months_same_year(){
    char *dateOfTravel = "03-11-2018";
    char *dateOfVaccination = "03-05-2018";
    TEST_ASSERT(dateDiff(dateOfTravel, dateOfVaccination) == 0);
}

void less_than_six_months_same_year(){
    char *dateOfTravel = "03-11-2018";
    char *dateOfVaccination = "03-06-2018";
    TEST_ASSERT(dateDiff(dateOfTravel, dateOfVaccination) == 0);
}

TEST_LIST = {
    {"year_diff", test_years_difference},
    {"six_months_diff_year", test_six_months_diff_year},
    {"less_than_six_months_diff_year", test_less_than_six_months_diff_year},
    {"six_months_same_year", six_months_same_year},
    {"more_than_six_months_same_year", more_than_six_months_same_year},
    {"exactly_six_months_same_year", exactly_six_months_same_year},
    {"less_than_six_months_same_year",less_than_six_months_same_year},
    {NULL, NULL}
};