#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/hashmap.h"
#include "../include/setofbfs.h"
#include "../include/virus.h"
#include "../include/country.h"
#include "../include/dateOps.h"

void travelRequest(hashMap *setOfBFs_map, hashMap *country_map, char *citizenID, char *dateOfTravel, char *countryName, char *virusName, int bufferSize, int *read_file_descs, int *write_file_descs){
  Country *curr_country;
	setofbloomfilters *curr_set = (setofbloomfilters *) find_node(setOfBFs_map, virusName);
  if(curr_set == NULL){
    printf("No such virus: %s\n", virusName);
    return;
  }else{
    // Find which monitor handled the country's subdirectory
    curr_country = (Country *) find_node(country_map, countryName);
    if(curr_country == NULL){
      printf("No such country: %s\n", countryName);
      return;
    }
    if(lookup_bf_vaccination(curr_set, curr_country->index, citizenID) == 1){
      char *request = malloc(255*sizeof(char));
      char *pipeReadBuffer = malloc(bufferSize*sizeof(char));
      char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
      char request_length, charsCopied, charsToWrite, charactersRead;
      sprintf(request, "checkSkiplist %s %s", citizenID, virusName);
      request_length = strlen(request);
      if(write(write_file_descs[curr_country->index], &request_length, sizeof(char))  < 0){
        perror("write checkSkiplist command length");
      }
      while(read(read_file_descs[curr_country->index], pipeReadBuffer, sizeof(char)) < 0);
      charsCopied = 0;
      while(charsCopied < request_length){
        if(request_length - charsCopied < bufferSize){
          charsToWrite = request_length - charsCopied;
        }else{
          charsToWrite = bufferSize;
        }
        strncpy(pipeWriteBuffer, request + charsCopied, charsToWrite*sizeof(char));
        if(write(write_file_descs[curr_country->index], pipeWriteBuffer, charsToWrite) < 0){
          perror("write checkSkiplist command chunk");
        }else{
          charsCopied += charsToWrite;
        }
      }
      // Waiting for length of response.
      while(read(read_file_descs[curr_country->index], &request_length, sizeof(char)) < 0);
      if(write(write_file_descs[curr_country->index], "1", sizeof(char)) < 0){
        perror("write answer length reception confirmation");
      }
      // Reading actual answer, byte by byte.
      charsCopied = 0;
      memset(request, 0, 255*sizeof(char));
      while(charsCopied < request_length){
        if((charactersRead  = read(read_file_descs[curr_country->index], pipeReadBuffer, bufferSize)) < 0){
          continue;
        }else{
          strncat(request, pipeReadBuffer, charactersRead);
          charsCopied += charactersRead;
        }
      }
      // Send confirmation of answer reception.
      if(write(write_file_descs[curr_country->index], "1", sizeof(char)) < 0){
        perror("write answer reception confirmation");
      }
      if(request_length == 2){
        printf("REQUEST REJECTED - YOU ARE NOT VACCINATED\n");
      }else{
        char *answer = malloc(4*sizeof(char));
        char *date = malloc(12*sizeof(char));
        sscanf(request, "%s %s", answer, date);
        switch(dateDiff(dateOfTravel, date)){
          case -1: printf("ERROR - TRAVEL DATE BEFORE VACCINATION DATE\n");
              break;
          case 0: printf("REQUEST ACCEPTED – HAPPY TRAVELS\n");
              break;
          case 1: printf("REQUEST REJECTED – YOU WILL NEED ANOTHER VACCINATION BEFORE TRAVEL DATE\n");
              break;
          default: printf("Invalid return value from date difference function.\n");
              break; 
        }
        free(answer);
        free(date);
      }
      free(request);
      free(pipeReadBuffer);
      free(pipeWriteBuffer);
    }else{
      printf("REQUESTED REJECTED - YOU ARE NOT VACCINATED\n");
    }
  } 
}