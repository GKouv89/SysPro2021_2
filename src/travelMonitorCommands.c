#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../include/hashmap.h"
#include "../include/setofbfs.h"
#include "../include/virus.h"
#include "../include/country.h"
#include "../include/dateOps.h"
#include "../include/requests.h"

int passCommandLineArgs(int readfd, char *writePipe, int bufferSize, int sizeOfBloom, char *input_dir){
  int writefd = open(writePipe, O_WRONLY);
  char inputDirLength, charsToWrite, charsCopied = 0;
  char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
  char *pipeReadBuffer = malloc(bufferSize*sizeof(char));
  if(writefd < 0){
    perror("open write pipe");
  }else{
    if(write(writefd, &bufferSize, sizeof(int)) < 0){
      perror("write bufferSize");
    }else{
      // Waiting for confirmation from the child for bufferSize
      while(read(readfd, pipeReadBuffer, bufferSize) < 0);
      // Letting child know size of bloomfilters
      if(write(writefd, &sizeOfBloom, sizeof(int)) < 0){
        perror("write sizeOfBloom length");
      }	
      // Waiting for confirmation from the child for sizeOfBloom
      while(read(readfd, pipeReadBuffer, bufferSize) < 0);
      // Letting monitors know input_dir's length and then name,
      // so they have the full path to their subdirectories.
      inputDirLength = strlen(input_dir); // Not an actual country yet.
      if(write(writefd, &inputDirLength, sizeof(char)) < 0){
        perror("write input_dir length");
      }else{
        while(charsCopied < inputDirLength){
          if(inputDirLength - charsCopied < bufferSize){
            charsToWrite = inputDirLength - charsCopied;
          }else{
            charsToWrite = bufferSize;
          }
          strncpy(pipeWriteBuffer, input_dir + charsCopied, charsToWrite);
          if(write(writefd, pipeWriteBuffer, charsToWrite) < 0){
            perror("write input_dir chunk");
          }
          charsCopied += charsToWrite;
        }
      }
    }
  }
  free(pipeWriteBuffer);
  free(pipeReadBuffer);
  return writefd;
}

void childReplacement(hashMap *country_map, pid_t oldChild, int **children_pids, int *read_file_descs, int **write_file_descs, int numMonitors, int bufferSize, int sizeOfBloom, int monitorIndex, char *input_dir){
  /* See which child (count-wise) was the one killed, spawn it and replace its process ID */
  int index = -1;
  int i;
  for(i = 0; i < numMonitors; i++){
    if(oldChild == (*children_pids)[i]){
      index = i;
      break;
    }
  }
  assert(index != -1);
  pid_t pid;
  char *path = "monitorProcess"; 
  char *readPipe = malloc(20*sizeof(char));
  char *writePipe = malloc(20*sizeof(char));
  if((pid = fork()) < 0){
    perror("fork");
    exit(1);
  }
  if(pid == 0){
    sprintf(readPipe, "pipes/%dr", index+1);
    sprintf(writePipe, "pipes/%dw", index+1);
    if(execlp(path, path, readPipe, writePipe, NULL) < 0){
      perror("execlp");
      exit(1);
    }
  }else{
    (*children_pids)[i] = pid;
  }
  /* Pass arguments (sizeOfBloom, bufferSize) to child  */
  (*write_file_descs)[index] = passCommandLineArgs(read_file_descs[index], writePipe, bufferSize, sizeOfBloom, input_dir);
  
  /* Search through map to find 
    all countries with index == monitorIndex
    and send country name through readfd and writefd */
  free(readPipe);
  free(writePipe);
}

void travelRequest(hashMap *setOfBFs_map, hashMap *country_map, char *citizenID, char *dateOfTravel, char *countryName, char *virusName, int bufferSize, int *read_file_descs, int *write_file_descs, requests *reqs){
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
        reqs->rejected++;
        reqs->total++;
      }else{
        char *answer = malloc(4*sizeof(char));
        char *date = malloc(12*sizeof(char));
        sscanf(request, "%s %s", answer, date);
        switch(dateDiff(dateOfTravel, date)){
          case -1: printf("ERROR - TRAVEL DATE BEFORE VACCINATION DATE\n");
              break;
          case 0: printf("REQUEST ACCEPTED – HAPPY TRAVELS\n");
              reqs->accepted++;
              reqs->total++;
              break;
          case 1: printf("REQUEST REJECTED – YOU WILL NEED ANOTHER VACCINATION BEFORE TRAVEL DATE\n");
              reqs->rejected++;
              reqs->total++;
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
      reqs->rejected++;
      reqs->total++;
      printf("REQUESTED REJECTED - YOU ARE NOT VACCINATED\n");
    }
  } 
}