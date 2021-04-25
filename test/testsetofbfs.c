#include "acutest.h"
#include "../include/setofbfs.h"
#include "../include/inputparsing.h"
#include "../include/hashmap.h"
#include "../include/virus.h"

struct query {
    int countryNumber;
    char *id;
    int answer;
    Virus *virus;
};

struct query queries[] = {
    [0] = {.countryNumber = 0, .id = "8946", .answer = 0},
    [1] = {.countryNumber = 1, .id = "1543", .answer = 0}
};


void test_setofbfs_creation_destruction(){
    setofbloomfilters *sbf;
    char *virusName = "DENGUE";
    create_setOfBFs(&sbf, virusName, 3, 100000);
    TEST_ASSERT(sbf != NULL);
    // Add 3 empty bloomfilters, and then call destroy
    for(int i = 0; i < 3; i++){
        TEST_ASSERT(sbf->bfs[i] == NULL);
        add_BFtoSet(sbf, i);
        TEST_ASSERT(sbf->bfs[i] != NULL);
    }
    destroy_setOfBFs(&sbf);
    TEST_ASSERT(sbf == NULL);
}

void test_seperate_file_bloomfilter_creation(){
    hashMap *virus_map_1, *virus_map_2, *country_map_1, *country_map_2, *citizen_map_1, *citizen_map_2;
    create_map(&virus_map_1, 1, Virus_List);
    create_map(&country_map_1, 1, Country_List);
    create_map(&citizen_map_1, 1, Citizen_List);
    create_map(&virus_map_2, 1, Virus_List);
    create_map(&country_map_2, 1, Country_List);
    create_map(&citizen_map_2, 1, Citizen_List);


    // Reading Andorra and Cyprus' files, that 'monitor 1' == map_1 handles.
    FILE *monitor1f = fopen("test/test_input_dir/Andorra-Polio.txt", "r");
    TEST_ASSERT(monitor1f != NULL);
    inputFileParsing(country_map_1, citizen_map_1, virus_map_1, monitor1f, 100000);
    TEST_ASSERT(fclose(monitor1f) == 0);
    monitor1f = fopen("test/test_input_dir/Cyprus-Polio.txt", "r");
    TEST_ASSERT(monitor1f != NULL);
    inputFileParsing(country_map_1, citizen_map_1, virus_map_1, monitor1f, 100000);
    TEST_ASSERT(fclose(monitor1f) == 0);

    // Reading China's file, that 'monitor 2' == map_2 handles.
    FILE *monitor2f = fopen("test/test_input_dir/China-Polio.txt", "r");
    TEST_ASSERT(monitor2f != NULL);
    inputFileParsing(country_map_2, citizen_map_2, virus_map_2, monitor2f, 100000);
    TEST_ASSERT(fclose(monitor2f) == 0);

    // Actual test about to begin.
    setofbloomfilters *sbf;
    char *virusName = "POLIO";
    create_setOfBFs(&sbf, virusName, 2, 100000);
    for(int i = 0; i < 2; i++){
        add_BFtoSet(sbf, i);
    }
    // Copying bloomfilters (THIS WILL CHANGE TO THE READ FUNCTION BUT NOT READY YET)
    memcpy(&(sbf->bfs[0]->hash_func_amount), &(((Virus *)virus_map_1->map[0]->bl->front->content)->virusBF->hash_func_amount), sizeof(int));
    memcpy(&(sbf->bfs[1]->hash_func_amount), &(((Virus *)virus_map_2->map[0]->bl->front->content)->virusBF->hash_func_amount), sizeof(int));
    memcpy(&(sbf->bfs[0]->size), &(((Virus *)virus_map_1->map[0]->bl->front->content)->virusBF->size), sizeof(int));
    memcpy(&(sbf->bfs[1]->size), &(((Virus *)virus_map_2->map[0]->bl->front->content)->virusBF->size), sizeof(int));
    memcpy(sbf->bfs[0]->filter, ((Virus *)virus_map_1->map[0]->bl->front->content)->virusBF->filter, 100000*sizeof(char));
    memcpy(sbf->bfs[1]->filter, ((Virus *)virus_map_1->map[0]->bl->front->content)->virusBF->filter, 100000*sizeof(char));
    // Running series of queries.
    queries[0].virus = (Virus *)virus_map_1->map[0]->bl->front->content;
    queries[1].virus = (Virus *)virus_map_2->map[0]->bl->front->content;
    for(int i = 0; i < 2; i++){
        TEST_ASSERT(lookup_bf_vaccination(sbf, queries[i].countryNumber, queries[i].id) == lookup_in_virus_bloomFilter(queries[i].virus, queries[i].id));
    }
    // Freeing all other data structures, to have only what the parent would have.
    destroy_map(&virus_map_1);
    destroy_map(&virus_map_2);
    destroy_map(&country_map_1);
    destroy_map(&country_map_2);
    destroy_map(&citizen_map_1);
    destroy_map(&citizen_map_2);
    destroy_setOfBFs(&sbf);
}

TEST_LIST = {
    {"creation_destruction", test_setofbfs_creation_destruction},
    {"lookup_one_virus", test_seperate_file_bloomfilter_creation},
    {NULL, NULL}
};