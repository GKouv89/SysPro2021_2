#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#include "../include/inputparsing.h"
#include "../include/hashmap.h"
#include "../include/skiplist.h"
#include "../include/monitorProcessCommands.h"
#include "../include/requests.h"

extern int errno;

int sigToHandle = 0;

void setSigToHandle(){
	sigToHandle = 1;
}

void checkForExit(char **countries, char countryIndex, requests *reqs){
	if(sigToHandle){
		prematureExit(countries, countryIndex, reqs);
	}
}

int main(int argc, char *argv[]){
	// Setting everything up for abrupt exit of child through SIGINT/SIGQUIT via terminal...
	static struct sigaction act;
	act.sa_handler=setSigToHandle;
	sigfillset(&(act.sa_mask));
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGQUIT, &act, NULL);
	requests reqs = {0, 0, 0};

	// Arguments passed through exec:
	// An ID number: it's the number of the monitor process
	// from the parent's perspective. Needed so the monitor
	// know which pipes/Xw to write, where X is said number

	// Open the appropriate pipe for current process to read and parent process to write.
	char *writePipeName = malloc(11*sizeof(char));
	char *readPipeName = malloc(11*sizeof(char));
	strcpy(readPipeName, argv[2]);
	strcpy(writePipeName, argv[1]);
	int readfd = open(readPipeName, O_RDONLY | O_NONBLOCK);
	// Open the appropriate pipe created by parent for them to read and current process to write.
	int writefd = open(writePipeName, O_WRONLY);
	int bufferSize, sizeOfBloom;
	// Block until bufferSize is sent from parent. 
	fd_set rd;
	FD_ZERO(&rd);
	FD_SET(readfd, &rd);
	if(select(readfd+1, &rd, NULL, NULL, NULL) == -1){
		perror("select");
	}
	if(FD_ISSET(readfd, &rd)){ 
		if(read(readfd, &bufferSize, sizeof(int)) < 0){
			perror("read");
		}else{
			if(write(writefd, "1", sizeof(char)) < 0){
				perror("confirmation for bufferSize write");
			}
		}
	}
	// Block until sizeOfBloom is sent from parent. 
	if(select(readfd+1, &rd, NULL, NULL, NULL) == -1){
		perror("select");
	}
	if(FD_ISSET(readfd, &rd)){ 
		if(read(readfd, &sizeOfBloom, sizeof(int)) < 0){
			perror("read");
		}
	}
	// About to read country names that this monitor will process
	// If there are more, we will resize the array.
	char **countries;
	int countryIndex = 0;
	int countriesLength = 10;
	countries = malloc(countriesLength*sizeof(char *)); // we assume that each monitor will read 10 directories.
	int i;
	for(i = 0; i < countriesLength; i++){
		countries[i] = calloc(255, sizeof(char));
	}
	int charactersRead, charactersParsed = 0, done = 0;
	char currCountryLength;
	char *readPipeBuffer = malloc(bufferSize*sizeof(char));
	while(!done){
		// Send confirmation to parent that
		// either the bufferSize was received OK and therefore
		// parent can send first country name or
		// latest country name was received OK and therefore
		// parent can send the next one.
		// The first country name truly received is the input_dir directory's name
		// necessary in order to have the full path to the subdirectories.
		if(write(writefd, "1", sizeof(char)) < 0){
			perror("confirmation write");
		}
		// First thing read is the length of the first country name. 
		if(select(readfd+1, &rd, NULL, NULL, NULL) == -1){
			perror("country length select");
			done = 1;
		}
		if(FD_ISSET(readfd, &rd)){
			if(read(readfd, &currCountryLength, sizeof(char)) < 0){
				perror("country length read");
			}else{
				while(charactersParsed < currCountryLength){
					if((charactersRead = read(readfd, readPipeBuffer, bufferSize)) < 0){
						// This means that the parent isn't done writing the chunk to the pipe,
						// but it will be ready shortly, so we just move on to the next rep.
						continue;
					}else{
						strncat(countries[countryIndex], readPipeBuffer, charactersRead);
						charactersParsed+=charactersRead;
					}		
				}
				if(strcmp(countries[countryIndex], "END") == 0){
					memset(countries[countryIndex], 0, 3);
					done = 1;
					break;
				}
				countryIndex++;
				charactersParsed = 0;
				// If need be, we allocate more space for next country names
				if(countryIndex == countriesLength){
					char **temp = realloc(countries, 2*countriesLength);
					for(i = countriesLength; i < 2*countriesLength; i++){
						countries[i] = calloc(255, sizeof(char));
					}
					assert(temp != NULL);
					countries = temp;
					countriesLength *= 2;
				}
			}
		}	
	}
	// Making the assumption that the SIGINT/SIGQUIT will be received AFTER the country names have been received,
	// as they should be included in the log file, this is the first of many periodic checks for a reception of
	// such a signal.
	// These checks will take place after the completion of operations; they will not interrupt them
	checkForExit(countries, countryIndex, &reqs);

	// First, a hashmap for each of the following: virus, country, citizen.
	// Arguments to inputParsing, the function that will initialize our data structures with the data.
	// in the input files.

	///////////////////////////////////////////////////////////////
	hashMap *country_map, *virus_map, *citizen_map;
	// Prime bucket of numbers for all hashmaps
	// virusesFile has only about 12 viruses
	// and countriesFile contains 195 countries
	// so the primes were chosen to be an order of magnitude
	// smaller than the size of the respective file
	// for citizens, perhaps the input file will have few lines,
	// but perhaps it will have upwards of 1000 records.
	// In any case, this allows for insertion of multiple records
	// after reading the input file
	create_map(&country_map, 43, Country_List);
	create_map(&virus_map, 3, Virus_List);
	create_map(&citizen_map, 101, Citizen_List);
	//////////////////////////////////////////////////////////////// 

	// Read files from subdirectories, create data structures. 
	char *full_file_name = malloc(512*sizeof(char));
	char *working_dir = malloc(512*sizeof(char));
	struct dirent *curr_subdir;
	FILE *curr_file;
	for(i = 1; i < countryIndex; i++){
		strcpy(working_dir, countries[0]);
		strcat(working_dir, "/");
		strcat(working_dir, countries[i]);
		DIR *work_dir = opendir(working_dir);
		curr_subdir = readdir(work_dir);
		while(curr_subdir != NULL){
			if(strcmp(curr_subdir->d_name, ".") == 0 || strcmp(curr_subdir->d_name, "..") == 0){
				curr_subdir = readdir(work_dir);
				continue;
			}
			strcpy(full_file_name, working_dir);
			strcat(full_file_name, "/");
			strcat(full_file_name, curr_subdir->d_name);
			curr_file = fopen(full_file_name, "r");
			assert(curr_file != NULL);
			inputFileParsing(country_map, citizen_map, virus_map, curr_file, sizeOfBloom);
			assert(fclose(curr_file) == 0);			
			curr_subdir = readdir(work_dir);
		}
		closedir(work_dir);
	}
  	send_bloomFilters(virus_map, readfd, writefd, bufferSize);
	checkForExit(countries, countryIndex, &reqs);

  char commandLength, charactersCopied, charsToWrite; 
  char *command = calloc(255, sizeof(char));
  char *command_name, *rest;
  char *citizenID = malloc(5*sizeof(char));
  char *virusName = malloc(50*sizeof(char));
  char *writePipeBuffer = malloc(bufferSize*sizeof(char));
  while(1){
	// Checking to see if SIGINT or SIGQUIT was received during the immediate previous operation.
	checkForExit(countries, countryIndex, &reqs);
    FD_ZERO(&rd);
    FD_SET(readfd, &rd);
    if(select(readfd + 1, &rd, NULL, NULL, NULL) < 1 && errno == EINTR){
		// Checking to see if SIGINT or SIGQUIT was received while waiting for a request.
		checkForExit(countries, countryIndex, &reqs);
	//   perror("select child while waiting for command");
    }else{
      if(read(readfd, &commandLength, sizeof(char)) < 0){
        perror("read request length");
      }else{
        if(write(writefd, "1", sizeof(char)) < 0){
          perror("write request length confirmation");
        }else{
          memset(command, 0, 255*sizeof(char));
          charactersParsed = 0;
          while(charactersParsed < commandLength){
            if((charactersRead = read(readfd, readPipeBuffer, bufferSize)) < 0){
              continue;
            }else{
              strncat(command, readPipeBuffer, charactersRead*sizeof(char));
              charactersParsed += charactersRead;
            }
          }
          command_name = strtok_r(command, " ", &rest);
          if(strcmp(command_name, "checkSkiplist") == 0){
            if(sscanf(rest, "%s %s", citizenID, virusName) == 2){
              checkSkiplist(virus_map, citizenID, virusName, bufferSize, readfd, writefd, &reqs);
            }
          }else{
            printf("Unknown command in child: %s\n", command_name);
          }
        }
      }
    }
  }

	// Releasing resources...
	// for(i = 0; i < countriesLength; i++){
		// free(countries[i]);
		// countries[i] = NULL;
	// }

	// free(countries);
	// close(writefd);
	// close(readfd);
	// free(logFileName);
	// free(writePipeName);
	// free(readPipeName);	
	// free(writePipeBuffer);
	// free(readPipeBuffer);
	// free(full_file_name);
	// free(working_dir);
	// destroy_map(&country_map);
	// destroy_map(&citizen_map);
	// destroy_map(&virus_map);
	// printf("Exiting...\n");
	return 0;
}
