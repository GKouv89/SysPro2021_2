#include "acutest.h"
#include "../include/setofbfs.h"
#include "../include/inputparsing.h"
#include "../include/hashmap.h"
#include "../include/virus.h"

struct query {
    int bfNumber;
    char *id;
    char *virusName;
};

struct query queries[] = {
    [0] = {.bfNumber = 0, .id = "8946", .virusName = "POLIO"},
    [1] = {.bfNumber = 1, .id = "1543", .virusName = "POLIO"}
};

struct query mult_queries[] = {
    [0] = {.bfNumber = 0, .id = "1129", .virusName = "POLIO"},
    [1] = {.bfNumber = 0, .id = "1833", .virusName = "COVID-19"},
    [2] = {.bfNumber = 0, .id = "7032", .virusName = "COVID-19"},
    [3] = {.bfNumber = 1, .id = "210", .virusName = "POLIO"},
    [4] = {.bfNumber = 1, .id = "2575", .virusName = "POLIO"},
    [5] = {.bfNumber = 0, .id = "4844", .virusName = "COVID-19"},
    [6] = {.bfNumber = 0, .id = "3097", .virusName = "COVID-19"},
    [7] = {.bfNumber = 0, .id = "5207", .virusName = "COVID-19"},
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

void test_multiple_viruses_setOfBloomFilters_creation(){
    hashMap *virus_map_1, *virus_map_2, *country_map_1, *country_map_2, *citizen_map_1, *citizen_map_2;
    create_map(&virus_map_1, 1, Virus_List);
    create_map(&country_map_1, 1, Country_List);
    create_map(&citizen_map_1, 1, Citizen_List);
    create_map(&virus_map_2, 1, Virus_List);
    create_map(&country_map_2, 1, Country_List);
    create_map(&citizen_map_2, 1, Citizen_List);


    // Reading Andorra and Cyprus' files, that 'monitor 1' == map_1 handles.
    FILE *monitor1f = fopen("test/test_input_dir/Andorra-Mixed.txt", "r");
    TEST_ASSERT(monitor1f != NULL);
    inputFileParsing(country_map_1, citizen_map_1, virus_map_1, monitor1f, 100000);
    TEST_ASSERT(fclose(monitor1f) == 0);
    monitor1f = fopen("test/test_input_dir/Cyprus-Mixed.txt", "r");
    TEST_ASSERT(monitor1f != NULL);
    inputFileParsing(country_map_1, citizen_map_1, virus_map_1, monitor1f, 100000);
    TEST_ASSERT(fclose(monitor1f) == 0);

    // Reading China's file, that 'monitor 2' == map_2 handles.
    FILE *monitor2f = fopen("test/test_input_dir/China-Mixed.txt", "r");
    TEST_ASSERT(monitor2f != NULL);
    inputFileParsing(country_map_2, citizen_map_2, virus_map_2, monitor2f, 100000);
    TEST_ASSERT(fclose(monitor2f) == 0);

    // Actual test about to begin.
    setofbloomfilters *sbf, *sbf2;
    char *virusName = "POLIO", *virusName2 = "COVID-19";
    create_setOfBFs(&sbf, virusName, 2, 100000);
    for(int i = 0; i < 2; i++){
        add_BFtoSet(sbf, i);
    }
    create_setOfBFs(&sbf2, virusName2, 2, 100000);
    for(int i = 0; i < 2; i++){
        add_BFtoSet(sbf2, i);
    }
    // Copying filters.
    // SBF is the set for POLIO, SBF2 is the set for COVID-19
    // index 0 is for monitorf1 (Andorra, Cyprus)
    // index 1 is for monitorf2 (China)
    bucketNode *b1, *b2;
    b1 = virus_map_1->map[0]->bl->front;
    b2 = virus_map_2->map[0]->bl->front;
    for(int i = 0; i < 2; i++){
        if(b1 != NULL){
            if(strcmp(((Virus *)b1->content)->name, "POLIO") == 0){
                memcpy(&(sbf->bfs[0]->hash_func_amount), &(((Virus *)b1->content)->virusBF->hash_func_amount), sizeof(int));
                memcpy(&(sbf->bfs[0]->size), &(((Virus *)b1->content)->virusBF->size), sizeof(int));
                memcpy(sbf->bfs[0]->filter, ((Virus *)b1->content)->virusBF->filter, 100000*sizeof(char));
            }else{
                memcpy(&(sbf2->bfs[0]->hash_func_amount), &(((Virus *)b1->content)->virusBF->hash_func_amount), sizeof(int));
                memcpy(&(sbf2->bfs[0]->size), &(((Virus *)b1->content)->virusBF->size), sizeof(int));
                memcpy(sbf2->bfs[0]->filter, ((Virus *)b1->content)->virusBF->filter, 100000*sizeof(char));
            }
            b1 = b1->next;
        }
        if(b2 != NULL){
            if(strcmp(((Virus *)b2->content)->name, "POLIO") == 0){
                memcpy(&(sbf->bfs[0]->hash_func_amount), &(((Virus *)b2->content)->virusBF->hash_func_amount), sizeof(int));
                memcpy(&(sbf->bfs[0]->size), &(((Virus *)b2->content)->virusBF->size), sizeof(int));
                memcpy(sbf->bfs[0]->filter, ((Virus *)b2->content)->virusBF->filter, 100000*sizeof(char));
            }else{
                memcpy(&(sbf2->bfs[0]->hash_func_amount), &(((Virus *)b2->content)->virusBF->hash_func_amount), sizeof(int));
                memcpy(&(sbf2->bfs[0]->size), &(((Virus *)b2->content)->virusBF->size), sizeof(int));
                memcpy(sbf2->bfs[0]->filter, ((Virus *)b2->content)->virusBF->filter, 100000*sizeof(char));
            }
            b2 = b2->next;
        }
    }

    Virus *v;
    for(int i = 0; i < 8; i++){
        if(mult_queries[i].bfNumber == 0){
            v = (Virus *) find_node(virus_map_1, mult_queries[i].virusName);
            if(v != NULL){
                if(strcmp(v->name, "POLIO") == 0){
                    TEST_ASSERT(lookup_bf_vaccination(sbf, mult_queries[i].bfNumber, mult_queries[i].id) == lookup_in_virus_bloomFilter(v, mult_queries[i].id));
                }else{
                    TEST_ASSERT(lookup_bf_vaccination(sbf2, mult_queries[i].bfNumber, mult_queries[i].id) == lookup_in_virus_bloomFilter(v, mult_queries[i].id));
                }
            }
        }else{
            v = (Virus *) find_node(virus_map_2, mult_queries[i].virusName);
            if(v != NULL){
                if(strcmp(v->name, "POLIO") == 0){
                    TEST_ASSERT(lookup_bf_vaccination(sbf, mult_queries[i].bfNumber, mult_queries[i].id) == lookup_in_virus_bloomFilter(v, mult_queries[i].id));
                }else{
                    TEST_ASSERT(lookup_bf_vaccination(sbf2, mult_queries[i].bfNumber, mult_queries[i].id) == lookup_in_virus_bloomFilter(v, mult_queries[i].id));
                }
            }
        }
    }
    // Freeing all other data structures, to have only what the parent would have.
    destroy_map(&virus_map_1);
    destroy_map(&virus_map_2);
    destroy_map(&country_map_1);
    destroy_map(&country_map_2);
    destroy_map(&citizen_map_1);
    destroy_map(&citizen_map_2);
    destroy_setOfBFs(&sbf);
    destroy_setOfBFs(&sbf2);
}

TEST_LIST = {
    {"creation_destruction", test_setofbfs_creation_destruction},
    {"lookup_two_viruses", test_multiple_viruses_setOfBloomFilters_creation},
    {NULL, NULL}
};