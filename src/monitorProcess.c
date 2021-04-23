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
	// int countriesLength = 10;
	// char **countries = malloc(countriesLength*sizeof(char *)); // we assume that each monitor will read 10 directories.
	// int i;
	// for(i = 0; i < countriesLength; i++){
	// 	countries[i] = calloc(255, sizeof(char));
	// }
	// int charactersRead, charactersParsed, countryIndex = 0, done = 0;
	// char *readPipeBuffer = malloc(bufferSize*sizeof(char));
	// while(!done){
	// 	// printf("One reading operation about to be done.\n");
	// 	if(select(readfd+1, &rd, NULL, NULL, NULL) == -1){
	// 		perror("select");
	// 		done = 1;
	// 	}
	// 	if(FD_ISSET(readfd)){
	// 		if((charactersRead = read(readfd, readPipeBuffer, bufferSize)) < 0){
	// 			perror("read");
	// 			done = 1;
	// 		}else{
	// 			for(i = 0; i < charactersRead; i++){
	// 				strncat(countries[countryIndex], readPipeBuffer + i, 1);
	// 				printf("Character parsed: %c\n", readPipeBuffer[i]);
	// 				// when we reach a terminal character, we have parsed one country name
	// 				// and we must move to the next
	// 				if(readPipeBuffer[i] == '\0'){
	// 					if(strcmp(countries[countryIndex], "END") == 0){
	// 						memset(countries[countryIndex], 0, 3);
	// 						countryIndex--;
	// 						done = 1;
	// 						break;
	// 					}
	// 					memmove(readPipeBuffer, readPipeBuffer + i, bufferSize - i);
	// 					countryIndex++;
	// 					if(countryIndex == countriesLength){
	// 						char **temp = realloc(countries, 2*countriesLength);
	// 						for(i = countriesLength; i < 2*countriesLength; i++){
	// 							countries[i] = calloc(255, sizeof(char));
	// 						}
	// 						assert(temp != NULL);
	// 						countries = temp;
	// 						countriesLength *= 2;
	// 					}
	// 				}
	// 			}
	// 		}
	// 	}
	// } 
	FILE *logfile = fopen(logFileName, "w");
	// for(i = 0; i < countryIndex; i++){
	// 	fprintf(logfile, "%s", countries[i]);
	// }
	fprintf(logfile, "bufferSize is: %d\n", bufferSize);
	assert(fclose(logfile) == 0);
	// for(i = 0; i < countriesLength; i++){
	// 	free(countries[i]);
	// 	countries[i] = NULL;
	// }
	// sleep(5);
	// free(countries);
	close(writefd);
	close(readfd);
	free(logFileName);
	free(writePipeName);
	free(readPipeName);	
	// free(writePipeBuffer);
	// free(readPipeBuffer);
	// printf("Exiting...\n");
	return 0;
}
