#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/hashmap.h"
#include "../include/bloomfilter.h"

void create_map(hashMap **map, int noOfBuckets, typeOfList type){
  (*map) = malloc(sizeof(hashMap));
  (*map)->noOfBuckets = noOfBuckets;
  (*map)->map = malloc(noOfBuckets*sizeof(hashBucket *));
  for(int i = 0; i < (*map)->noOfBuckets; i++){
    (*map)->map[i] = malloc(sizeof(hashBucket));
    create_bucketList(&((*map)->map[i]->bl), type);
  }
}

unsigned long hash_function(hashMap *map, unsigned char *str){
  unsigned long hash = djb2(str);
  return hash % map->noOfBuckets;
}

void insert(hashMap *map, unsigned char *key, void *content){
  unsigned long hash = hash_function(map, key);
  insert_bucketNode(map->map[hash]->bl, content);
}

void* find_node(hashMap *map, unsigned char *key){
  unsigned long hash = hash_function(map, key);
  search_bucketList(map->map[hash]->bl, key);
}

/////////////////////////////////////////////////////////////////
// This function is called only with the viruses hashmap       //
// for the first argument. Basically searches for each element //
// in each bucket, i.e. each virus, and sends the virus'       //
// bloom filter to the parent.                                 //
/////////////////////////////////////////////////////////////////

void send_bloomFilters(hashMap *map, int readfd, int writefd, int bufferSize){
    for(int i = 0; i < map->noOfBuckets; i++){
      send_virus_Bloomfilters(map->map[i]->bl, readfd, writefd, bufferSize);
    }
    char *endStr = "END";
    char charsCopied, endStrLen = 3;
    char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
    if(write(writefd, &endStrLen, sizeof(char)) < 0){
      perror("write END length");
    }else{ 
      charsCopied = 0;
      while(charsCopied < endStrLen){
        strncpy(pipeWriteBuffer, endStr + charsCopied, bufferSize);
        if(write(writefd, pipeWriteBuffer, bufferSize) < 0){
          perror("write END chunk");
        }
        charsCopied += bufferSize;
      }
    }
    free(pipeWriteBuffer);
}

void sendCountryNamesToChild(hashMap *map, int readfd, int writefd, int bufferSize, int monitorIndex){
  for(int i = 0; i < map->noOfBuckets; i++){
    sendCountriesToChild(map->map[i]->bl, readfd, writefd, bufferSize, monitorIndex);
  }
  char *endStr = "END";
  char charsCopied, endStrLen = 3;
  char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
  char *pipeReadBuffer = malloc(bufferSize*sizeof(char));

  fd_set rd;
  FD_ZERO(&rd);
  FD_SET(readfd, &rd);
  if(select(readfd + 1, &rd, NULL, NULL, NULL) == -1){
    perror("select for child ok confirmation");
  }else{
    // Read confirmation for OK reception of last subdirectory name.
    if(read(readfd, pipeReadBuffer, bufferSize) < 0){
      perror("read for child ok confirmation");
    }else{
      if(write(writefd, &endStrLen, sizeof(char)) < 0){
        perror("write END length");
      }else{
        charsCopied = 0;
        while(charsCopied < endStrLen){
          strncpy(pipeWriteBuffer, endStr + charsCopied, bufferSize);
          if(write(writefd, pipeWriteBuffer, bufferSize) < 0){
            perror("write END chunk");
          }
          charsCopied += bufferSize;
        }
      }
    }
  }
  free(pipeWriteBuffer);
  free(pipeReadBuffer);
}

void printSubdirectoryNames(hashMap *map, FILE *fp){
  for(int i = 0; i < map->noOfBuckets; i++){
    printSubdirNames(map->map[i]->bl, fp);
  }  
}

/////////////////////////////////////////////////////////////////
// This function is called only with the viruses hashmap       //
// for the first argument. Basically searches for each element //
// in each bucket, i.e. each virus, whether the citizen        //
// has been vaccinated for it and if so, when.                 //
/////////////////////////////////////////////////////////////////

// void lookup_vacStatus_all(hashMap *map, unsigned char *citizenID){
//   for(int i = 0; i < map->noOfBuckets; i++){
//     vacStatus_all(map->map[i]->bl, citizenID);
//   }
// }

/////////////////////////////////////////////////////////////////
// This function is called only with the countries hashmap     //
// for the first argument. Basically searched for each country //
// and for one specific virus, how many citizens have been     //
// vaccinated against the virus between the two argument dates //
/////////////////////////////////////////////////////////////////

// void lookup_popStatus_all(hashMap *map, int mode, Virus *v, char *startingDate, char *endingDate){
//   for(int i = 0; i < map->noOfBuckets; i++){
//     popStatus_all(map->map[i]->bl, mode, v, startingDate, endingDate);
//   }
// }

void destroy_map(hashMap **map){
  for(int i = 0; i < (*map)->noOfBuckets; i++){
    destroy_bucketList(&((*map)->map[i]->bl));
    free((*map)->map[i]);
    (*map)->map[i] = NULL;
  }
  free((*map)->map);
  (*map)->map = NULL;
  free(*map);
  *map = NULL;
}
