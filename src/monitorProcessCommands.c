#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "../include/hashmap.h"
#include "../include/virus.h"
#include "../include/requests.h"
#include "../include/readWriteOps.h"

void checkSkiplist(hashMap *virus_map, char *citizenID, char *virusName, int bufferSize, int readfd, int writefd, requests *reqs){
  Virus *virus = (Virus *) find_node(virus_map, virusName);
  listNode *check_for_vacc = lookup_in_virus_vaccinated_for_list(virus, atoi(citizenID));
  char *answer = malloc(255*sizeof(char));
  if(check_for_vacc != NULL){
    sprintf(answer, "YES %s", check_for_vacc->vaccinationDate);
    reqs->accepted++;
    reqs->total++;
  }else{
    sprintf(answer, "NO");
    reqs->rejected++;
    reqs->total++;
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

void checkVacc(hashMap *citizen_map, hashMap *virus_map, char *citizenID, int readfd, int writefd, int bufferSize){
  Citizen *citizen = (Citizen *) find_node(citizen_map, citizenID);
  char *citizenData = calloc(1024, sizeof(char));
  char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
  if(citizen == NULL){
    strcpy(citizenData, "NO SUCH CITIZEN");
    write_content(citizenData, &pipeWriteBuffer, writefd, bufferSize);
    free(pipeWriteBuffer);
    free(citizenData);
    return;
  }
  print_citizen(citizen, &citizenData);
  write_content(citizenData, &pipeWriteBuffer, writefd, bufferSize);
  char confirmation;
  while(read(readfd, &confirmation, sizeof(char)) < 0);
  lookup_vacStatus_all(virus_map, citizenID, readfd, writefd, bufferSize);
  free(citizenData);
  free(pipeWriteBuffer);
}

void prematureExit(int readfd, int writefd, char **countries, int countryIndex, requests *reqs){
  close(writefd);
	close(readfd);
  // Create log file of current process 
	pid_t mypid = getpid();
  char *logFileName = malloc(20*sizeof(char));
  sprintf(logFileName, "log_file.%d", mypid);
	FILE *logfile = fopen(logFileName, "w");
	for(int i = 1; i < countryIndex; i++){
		fprintf(logfile, "%s\n", countries[i]);
	}
  fprintf(logfile, "TOTAL TRAVEL REQUESTS %d\n", reqs->total);
  fprintf(logfile, "ACCEPTED %d\n", reqs->accepted);
  fprintf(logfile, "REJECTED %d\n", reqs->rejected);
	assert(fclose(logfile) == 0);
	exit(0);
}