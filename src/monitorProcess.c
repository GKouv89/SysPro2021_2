#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(int argc, char *argv[]){
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
	int bufferSize;
	// Wait for necessary information to be sent from parent, such as the bufferSize
	fd_set rd;
	FD_ZERO(&rd);
	FD_SET(readfd, &rd);
	if(select(readfd+1, &rd, NULL, NULL, NULL) == -1){
		perror("select");
	}
	if(FD_ISSET(readfd, &rd)){ 
		if(read(readfd, &bufferSize, sizeof(int)) < 0){
			perror("read");
		}
	}
	// Create log file of current process and wait to receive country names from parent.
	pid_t mypid = getpid();
	char *logFileName = malloc(14*sizeof(char));
	sprintf(logFileName, "log_file.%d", mypid);
	// About to read country names that this monitor will process
	// If there are more, we will resize the array.
	int countriesLength = 10;
	char **countries = malloc(countriesLength*sizeof(char *)); // we assume that each monitor will read 10 directories.
	int i;
	for(i = 0; i < countriesLength; i++){
		countries[i] = calloc(255, sizeof(char));
	}
	int charactersRead, charactersParsed = 0, countryIndex = 0, done = 0;
	char currCountryLength;
	char *readPipeBuffer = malloc(bufferSize*sizeof(char));
	int repCount = 0;
	while(!done && repCount < 15){
		// Send confirmation to parent that
		// either the bufferSize was received OK and therefore
		// parent can send first country name or
		// latest country name was received OK and therefore
		// parent can send the next one.
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
				// printf("Country length: %d\n", currCountryLength);
				while(charactersParsed < currCountryLength){
					if((charactersRead = read(readfd, readPipeBuffer, bufferSize)) < 0){
						// perror("country chunk read");
					}else{
						// printf("Country chunk: %s\n", readPipeBuffer);
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
				// printf("Next country");
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
		repCount++;
	} 
	FILE *logfile = fopen(logFileName, "w");
	for(i = 0; i < countryIndex; i++){
		fprintf(logfile, "%s\n", countries[i]);
	}
	assert(fclose(logfile) == 0);
	for(i = 0; i < countriesLength; i++){
		free(countries[i]);
		countries[i] = NULL;
	}
	// sleep(5);
	free(countries);
	close(writefd);
	close(readfd);
	free(logFileName);
	free(writePipeName);
	free(readPipeName);	
	// free(writePipeBuffer);
	free(readPipeBuffer);
	// printf("Exiting...\n");
	return 0;
}
