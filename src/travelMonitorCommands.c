#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "../include/hashmap.h"
#include "../include/setofbfs.h"
#include "../include/virus.h"
#include "../include/country.h"
#include "../include/dateOps.h"
#include "../include/requests.h"
#include "../include/virusRequest.h"
#include "../include/readWriteOps.h"

void passCommandLineArgs(int readfd, int writefd, int bufferSize, int sizeOfBloom, char *input_dir){
  unsigned int inputDirLength, charsToWrite, charsCopied = 0;
  char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
  char *pipeReadBuffer = malloc(bufferSize*sizeof(char));
  if(write(writefd, &bufferSize, sizeof(int)) < 0){
    perror("write bufferSize");
  }else{
    // Waiting for confirmation from the child for bufferSize
    while(read(readfd, pipeReadBuffer, sizeof(char)) < 0);
    // Letting child know size of bloomfilters
    if(write(writefd, &sizeOfBloom, sizeof(int)) < 0){
      perror("write sizeOfBloom length");
    }	
    // Waiting for confirmation from the child for sizeOfBloom
    while(read(readfd, pipeReadBuffer, bufferSize) < 0);
    // Letting monitors know input_dir's length and then name,
    // so they have the full path to their subdirectories.
    inputDirLength = strlen(input_dir); // Not an actual country yet.
    if(write(writefd, &inputDirLength, sizeof(int)) < 0){
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
  free(pipeWriteBuffer);
  free(pipeReadBuffer);
}

void receiveBloomFiltersFromChild(hashMap *setOfBFs_map, int readfd, int writefd, int index, int bufferSize, int numMonitors, int sizeOfBloom){
  unsigned int dataLength, charactersParsed;
  int charactersRead;
  char *virusName = calloc(255, sizeof(char));
  char *pipeReadBuffer = malloc(bufferSize*sizeof(char));
  char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
  setofbloomfilters *curr_set;
  while(1){
    if(read(readfd, &dataLength, sizeof(int)) < 0){
      // perror("read virus name length");
      continue;
    }else{
      charactersParsed = 0;
      while(charactersParsed < dataLength){
        if((charactersRead = read(readfd, pipeReadBuffer, bufferSize)) < 0){
          // This means that the monitor isn't done writing the chunk to the pipe,
          // but it will be ready shortly, so we just move on to the next rep.
          continue;
        }else{
          strncat(virusName, pipeReadBuffer, charactersRead);
          charactersParsed+=charactersRead;
        }		
      }
      if(strcmp(virusName, "END") == 0){
        break;
      }else{
        // printf("Received faux bloom filter for Virus %s from monitor %d\n", virusName, k);
        curr_set = (setofbloomfilters *) find_node(setOfBFs_map, virusName);
        if(curr_set == NULL){
          create_setOfBFs(&curr_set, virusName, numMonitors, sizeOfBloom);
          add_BFtoSet(curr_set, index);
          insert(setOfBFs_map, virusName, curr_set);
        }else{
          add_BFtoSet(curr_set, index);
        }
        if(write(writefd, "1", sizeof(char)) < 0){
          perror("write confirmation after receiving virus name");
        }
        read_BF(curr_set, readfd, writefd, index, bufferSize);
        memset(virusName, 0, 255*sizeof(char));
      }
    }
  }
  free(virusName);
  free(pipeReadBuffer);
  free(pipeWriteBuffer);
}

void childReplacement(hashMap *country_map, hashMap *setOfBFs_map, pid_t oldChild, int **children_pids, int *read_file_descs, int *write_file_descs, int numMonitors, int bufferSize, int sizeOfBloom, char *input_dir){
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
  /* Close pipes and reopen to see if SIGPIPE will be avoided. */
  close(read_file_descs[index]);
  close(write_file_descs[index]);

  pid_t pid;
  char *path = "./Monitor"; 
  char *readPipe = malloc(20*sizeof(char));
  char *writePipe = malloc(20*sizeof(char));
  sprintf(readPipe, "/tmp/%dr", index+1);
  sprintf(writePipe, "/tmp/%dw", index+1);
  /* Reopen reading pipe. */
  read_file_descs[index] = open(readPipe, O_RDONLY | O_NONBLOCK);
  if(read_file_descs[index] < 0){
    perror("open read pipe");
  }
  if((pid = fork()) < 0){
    perror("fork");
    exit(1);
  }
  if(pid == 0){
    if(execl(path, path, readPipe, writePipe, NULL) < 0){
      perror("execlp");
      exit(1);
    }
  }else{
    (*children_pids)[index] = pid;
    // printf("New child: %d\n", (*children_pids)[index]);
  }
  /* Reopening writing pipe */
  write_file_descs[index] = open(writePipe, O_WRONLY);
  if(write_file_descs[index] < 0){
    perror("open write pipe");
  }
  
  /* Pass arguments (sizeOfBloom, bufferSize) to child  */
  passCommandLineArgs(read_file_descs[index], write_file_descs[index], bufferSize, sizeOfBloom, input_dir);
  char conf;
  while(read(read_file_descs[index], &conf, sizeof(char)) < 0);

  /* Search through map to find 
    all countries with index == monitorIndex
    and send country name through readfd and writefd */
  sendCountryNamesToChild(country_map, read_file_descs[index], write_file_descs[index], bufferSize, index);

  /* Receive bloom filters */
  receiveBloomFiltersFromChild(setOfBFs_map, read_file_descs[index], write_file_descs[index], index, bufferSize, numMonitors, sizeOfBloom);
  printf("Ready to accept requests again.\n");
  free(readPipe);
  free(writePipe);
}

void travelRequest(hashMap *setOfBFs_map, hashMap *country_map, hashMap *virusRequest_map, char *citizenID, char *dateOfTravel, char *countryName, char *countryTo, char *virusName, int bufferSize, int *read_file_descs, int *write_file_descs, requests *reqs){
  Country *curr_country;
	setofbloomfilters *curr_set = (setofbloomfilters *) find_node(setOfBFs_map, virusName);
  if(curr_set == NULL){
    printf("No such virus: %s\n", virusName);
    return;
  }else{
    // Have we received any requests for this virus yet? If not, time
    // to create the appropriate 'object' 
    virusRequest *vr = (virusRequest *) find_node(virusRequest_map, virusName);
    if(vr == NULL){
      create_virusRequest(&vr, virusName);
      insert(virusRequest_map, virusName, vr);
    }
    // Now that we definitely have a virusRequest, let's see if there has been any
    // other requests for travel to the same country on the same date. If not, 
    // let's create a namedRequest. addRequest does both the check and (if needed) the creation.
    namedRequests *nreq = addRequest(vr, countryTo, dateOfTravel);
    // Find which monitor handled the country's subdirectory
    curr_country = (Country *) find_node(country_map, countryName);
    if(curr_country == NULL){
      printf("No such country: %s\n", countryName);
      return;
    }
    if(lookup_bf_vaccination(curr_set, curr_country->index, citizenID) == 1){
      char *request = malloc(1024*sizeof(char));
      char *pipeReadBuffer = malloc(bufferSize*sizeof(char));
      char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
      unsigned int request_length;
      sprintf(request, "checkSkiplist %s %s %s", citizenID, virusName, countryName);
      write_content(request, &pipeWriteBuffer, write_file_descs[curr_country->index], bufferSize);
      memset(request, 0, 1024*sizeof(char));
      read_content(&request, &pipeReadBuffer, read_file_descs[curr_country->index], bufferSize);
      // Send confirmation of answer reception.
      if(write(write_file_descs[curr_country->index], "1", sizeof(char)) < 0){
        perror("write answer reception confirmation");
      }
      request_length = strlen(request);
      if(request_length == 2){
        printf("REQUEST REJECTED - YOU ARE NOT VACCINATED\n");
        reqs->rejected++;
        reqs->total++;
      }else if(strcmp(request, "BAD COUNTRY") == 0){
        printf("REQUEST REJECTED - INVALID COUNTRYFROM ARGUMENT FOR CITIZEN %s\n", citizenID);
        // This is NOT counted towards the overall requests, as the arguments were invalid.
      }else{
        char *answer = malloc(4*sizeof(char));
        char *date = malloc(12*sizeof(char));
        sscanf(request, "%s %s", answer, date);
        memset(request, 0, 1024*sizeof(char));
        switch(dateDiff(dateOfTravel, date)){
          case -1: printf("ERROR - TRAVEL DATE BEFORE VACCINATION DATE\n");
              strcpy(request, "REJECT");
              write_content(request, &pipeWriteBuffer, write_file_descs[curr_country->index], bufferSize);
              rejectNamedRequest(nreq);
              reqs->rejected++;
              reqs->total++;
              break;
          case 0: printf("REQUEST ACCEPTED – HAPPY TRAVELS\n");
              strcpy(request, "ACCEPT");
              write_content(request, &pipeWriteBuffer, write_file_descs[curr_country->index], bufferSize);
              acceptNamedRequest(nreq);
              reqs->accepted++;
              reqs->total++;
              break;
          case 1: printf("REQUEST REJECTED – YOU WILL NEED ANOTHER VACCINATION BEFORE TRAVEL DATE\n");
              strcpy(request, "REJECT");
              write_content(request, &pipeWriteBuffer, write_file_descs[curr_country->index], bufferSize);
              rejectNamedRequest(nreq);
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
      rejectNamedRequest(nreq);
      reqs->rejected++;
      reqs->total++;
      printf("REQUESTED REJECTED - YOU ARE NOT VACCINATED\n");
    }
  } 
}

void addVaccinationRecords(hashMap *country_map, hashMap *setOfBFs_map, char *countryName, pid_t *children_pids, int *read_file_descs, int *write_file_descs, int bufferSize, int numMonitors, int sizeOfBloom){
  Country *country = (Country *) find_node(country_map, countryName);
  if(country == NULL){
    printf("No such country: %s\n", countryName);
    return;
  }
  int index = country->index;
  // printf("Child %d is in charge of subdirectory: %s\n", index, countryName);
  kill(children_pids[index], 10);
  // Sending directory name through pipe...
  unsigned int countryLength = strlen(countryName);
  char confirmation;
  while(read(read_file_descs[index], &confirmation, sizeof(char)) < 0);
  if(write(write_file_descs[index], &countryLength, sizeof(int)) < 0){
    perror("write name of directory with new files");
  }else{
    unsigned int charsCopied = 0, charsToWrite = 0;
    char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
    while(charsCopied < countryLength){
      if(countryLength - charsCopied < bufferSize){
        charsToWrite = countryLength - charsCopied;
      }else{
        charsToWrite = bufferSize;
      }
      strncpy(pipeWriteBuffer, countryName + charsCopied, charsToWrite);
      if(write(write_file_descs[index], pipeWriteBuffer, charsToWrite*sizeof(char)) < 0 ){
        perror("write chunk of directory with new files");
      }else{
        charsCopied += charsToWrite;
      }
    }
    free(pipeWriteBuffer);
  }
  // Now waiting to receive updated bloom filters from child.
  receiveBloomFiltersFromChild(setOfBFs_map, read_file_descs[index], write_file_descs[index], index, bufferSize, numMonitors, sizeOfBloom);
  printf("Bloom filters updated. Ready to accept more requests.\n");
}

void noMoreCommands(struct sigaction *act, int numMonitors, pid_t *children_pids, hashMap *country_map, requests *reqs){
  act->sa_handler=SIG_DFL;
  sigaction(SIGCHLD, act, NULL);
  int i;
  for(i = 0; i < numMonitors; i++){
    kill(children_pids[i], 9);
  }
  // waiting for children to exit...
  // printf("About to wait for my kids...\n");
  for(i = 1; i <= numMonitors; i++){
    printf("Waiting for kid no %d\n", i);
    if(wait(NULL) == -1){
      perror("wait");
      exit(1);
    }
  }
  // Making log file.
  pid_t mypid = getpid();
	char *logfile = malloc(20*sizeof(char));
	sprintf(logfile, "log_file.%d", mypid);
	FILE *log = fopen(logfile, "w");
	assert(log != NULL);
	printSubdirectoryNames(country_map, log);
	fprintf(log, "TOTAL TRAVEL REQUESTS %d\nACCEPTED %d\nREJECTED %d\n", reqs->total, reqs->accepted, reqs->rejected);
	assert(fclose(log) == 0);
	free(logfile);
}

void searchVaccinationStatus(int *read_file_descs, int *write_file_descs, int numMonitors, int bufferSize, char *citizenID){
  unsigned int commandLength, charsWritten, charsToWrite;
  char *command = malloc(15*sizeof(char));
  char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
  char confirmation;
  sprintf(command, "checkVacc %s", citizenID);
  commandLength = strlen(command);
  int i;
  for(i = 0; i < numMonitors; i++){
    if(write(write_file_descs[i], &commandLength, sizeof(int)) < 0){
      perror("write checkVacc length");
    }else{
      charsWritten = 0;
      while(charsWritten < commandLength){
        if(commandLength - charsWritten < bufferSize){
          charsToWrite = commandLength - charsWritten;
        }else{
          charsToWrite = bufferSize;
        }
        strncpy(pipeWriteBuffer, command + charsWritten, charsToWrite);
        if(write(write_file_descs[i], pipeWriteBuffer, charsToWrite*sizeof(char)) < 0){
          perror("write checkVacc command chunk");
        }else{
            charsWritten += charsToWrite;
        }
      }
    }
  }

  fd_set rd, rd_specific;
  int *valid_read_file_descs = malloc(numMonitors*sizeof(int));
  int max = 0;
  FD_ZERO(&rd);
	for(i = 0; i < numMonitors; i++){
		FD_SET(read_file_descs[i], &rd);
    valid_read_file_descs[i] = 1;
    if(read_file_descs[i] > max){
      max = read_file_descs[i];
    }
	}
  max++;
  char charsCopied, charsRead, virusLength, answerLength;
  char *citizenData = calloc(1024, sizeof(char));
  char *virus = calloc(255, sizeof(char));
  char *answer = calloc(255, sizeof(char));
  char *pipeReadBuffer = malloc(bufferSize*sizeof(char));
  char *ans = malloc(4*sizeof(char));
  char *date = malloc(11*sizeof(char));
  int citizenFound = 0;
  for(i = 0; i < numMonitors; ){
    if(select(max, &rd, NULL, NULL, NULL) == -1){
      perror("select checkVacc response");
    }else{
      for(int j = 0; j < numMonitors; j++){
        if(valid_read_file_descs[j] == 1 && FD_ISSET(read_file_descs[j], &rd)){
          read_content(&citizenData, &pipeReadBuffer, read_file_descs[j], bufferSize);
          if(strcmp(citizenData, "NO SUCH CITIZEN") == 0){
            memset(citizenData, 0, 255*sizeof(char));  
          }else{
            printf("%s", citizenData);
            if(write(write_file_descs[j], "1", sizeof(char)) < 0){
              perror("couldn't confirm reception of citizenData in searchVaccinationStatus");
            }
            while(1){
              read_content(&virus, &pipeReadBuffer, read_file_descs[j], bufferSize);
              if(strcmp(virus, "END") == 0){
                break;
              }
              citizenFound = 1;
              if(write(write_file_descs[j], "1", sizeof(char)) < 0){
                perror("couldn't confirm reception of virus in searchVaccinationStatus");
              }else{
                // wish to block only to wait trafic from this specific monitor 
                FD_ZERO(&rd_specific);
                FD_SET(read_file_descs[j], &rd_specific);
                if(select(read_file_descs[j] + 1, &rd_specific, NULL, NULL, NULL) == -1){
                  perror("select checkVacc answer reception");
                }else{
                  read_content(&answer, &pipeReadBuffer, read_file_descs[j], bufferSize);
                  if(strlen(answer) == 2){
                    printf("%s NOT YET VACCINATED\n", virus);
                  }else{
                    sscanf(answer, "%s %s", ans, date);
                    printf("%s VACCINATED ON %s\n", virus, date);
                  }
                  memset(virus, 0, 255*sizeof(char));
                  memset(answer, 0, 255*sizeof(char));
                  memset(citizenData, 0, 1024*sizeof(char));
                  if(write(write_file_descs[j], "1", sizeof(char)) < 0){
                    perror("couldn't confirm reception of answer in searchVaccinationStatus");
                  }
                }
              }
            }
          }
          valid_read_file_descs[j] = 0;
          i++;
        }
      }
      // Response will be received from ALL monitors. At most one will have a valid answer.
      // In any case, we must reset the set of file_descriptors we expect traffic from.     
      max=0;
      FD_ZERO(&rd);
      for(int k = 0; k < numMonitors; k++){
        if(valid_read_file_descs[k] == 1){
          FD_SET(read_file_descs[k], &rd);
          if(read_file_descs[k] > max){
            max = read_file_descs[k];
          }
        }
      }
      max++;
    }
  }
  if(citizenFound == 0){
    printf("No citizen with id %s\n", citizenID);
  }
  free(citizenData);
  free(command);
  free(pipeWriteBuffer);
  free(pipeReadBuffer);
  free(valid_read_file_descs);
  free(virus);
  free(answer);
  free(ans);
  free(date);
}

void travelStats(hashMap *virusRequest_map, char *virusName, char *date1, char *date2, char *countryTo, int mode){
  virusRequest *vr = (virusRequest *) find_node(virusRequest_map, virusName);
  requests reqs = {0, 0, 0};
  if(vr == NULL){
    printf("No such virus: %s. Try again.\n", virusName);
  }else{
    switch(mode){
      case 0: gatherStatistics(vr, date1, date2, NULL, mode, &reqs);
              break;
      case 1: gatherStatistics(vr, date1, date2, countryTo, mode, &reqs);
              break;
      default:printf("Invalid function mode.\n");
              break;
    }
    printf("TOTAL REQUESTS: %d\n", reqs.total);
    printf("ACCEPTED: %d\n", reqs.accepted);
    printf("REJECTED: %d\n", reqs.rejected);
  }
}

