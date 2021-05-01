#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/hashmap.h"
#include "../include/virus.h"

void checkSkiplist(hashMap *virus_map, char *citizenID, char *virusName, int bufferSize, int readfd, int writefd){
  Virus *virus = (Virus *) find_node(virus_map, virusName);
  listNode *check_for_vacc = lookup_in_virus_vaccinated_for_list(virus, atoi(citizenID));
  char *answer = malloc(255*sizeof(char));
  if(check_for_vacc != NULL){
    sprintf(answer, "YES %s", check_for_vacc->vaccinationDate);
  }else{
    sprintf(answer, "NO");
  }
  char answerLength = strlen(answer);
  char *readPipeBuffer = malloc(bufferSize*sizeof(char));
  char *writePipeBuffer = malloc(bufferSize*sizeof(char));
  if(write(writefd, &answerLength, sizeof(char)) < 0){
    perror("write answer length confirmation");
  }else{
    while(read(readfd, readPipeBuffer, sizeof(char)) < 0);
    char charsToWrite, charactersCopied = 0;
    while(charactersCopied < answerLength){
      if(answerLength - charactersCopied < bufferSize){
        charsToWrite = answerLength - charactersCopied;
      }else{
        charsToWrite = bufferSize;
      }
      strncpy(writePipeBuffer, answer + charactersCopied, charsToWrite*sizeof(char));
      if(write(writefd, writePipeBuffer, charsToWrite) < 0){
        perror("write answer chunk");
      }else{
        charactersCopied += charsToWrite;
      }
    }         
  }
  while(read(readfd, readPipeBuffer, sizeof(char)) < 0);
  free(readPipeBuffer);
  free(writePipeBuffer);
}
