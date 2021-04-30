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

char **countries;
int acceptedRequests = 0;
int rejectedRequests = 0;
int totalRequests = 0;
int countryIndex = 0;

void closingHandler(int);

int main(int argc, char *argv[]){
  signal(SIGKILL, closingHandler);
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

  sleep(20);

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

void closingHandler(int signum){
  // Create log file of current process 
	pid_t mypid = getpid();
  char *logFileName = malloc(20*sizeof(char));
  sprintf(logFileName, "log_file.%d", mypid);
	FILE *logfile = fopen(logFileName, "w");
	for(int i = 1; i < countryIndex; i++){
		fprintf(logfile, "%s\n", countries[i]);
	}
  fprintf(logfile, "TOTAL TRAVEL REQUESTS %d\n", totalRequests);
  fprintf(logfile, "ACCEPTED %d\n", acceptedRequests);
  fprintf(logfile, "REJECTED %d\n", rejectedRequests);
	assert(fclose(logfile) == 0);
	exit(0);
}