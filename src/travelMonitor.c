#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include "../include/setofbfs.h"
#include "../include/country.h"
#include "../include/hashmap.h"
#include "../include/dateOps.h"
#include "../include/travelMonitorCommands.h"
#include "../include/requests.h"

extern int errno;

pid_t childThatExited;
int childHasExited = 0;

void childExited(){
	childHasExited = 1;
	childThatExited = wait(NULL);
}

void hasChildExited(hashMap *country_map, hashMap *setOfBFs_map, int **children_pids, int *read_file_descs, int *write_file_descs, int numMonitors, int bufferSize, int sizeOfBloom, char *input_dir){
	if(childHasExited){
		childHasExited = 0;
		// spawn child function, that will replace initialization code.
		childReplacement(country_map, setOfBFs_map, childThatExited, children_pids, read_file_descs, write_file_descs, numMonitors, bufferSize, sizeOfBloom, input_dir);
	}
}

int main(int argc, char *argv[]){
	// Setting everything up for abrupt exit of child through SIGINT/SIGQUIT via terminal, and the parent's
	// responsibility to spawn another one.
	static struct sigaction act;
	act.sa_handler=childExited;
	sigfillset(&(act.sa_mask));
	sigaction(SIGCHLD, &act, NULL);
	requests reqs = {0, 0, 0};

	if(argc != 9){
		printf("Usage: ./travelMonitor -m numMonitors -b bufferSize -s sizeOfBloom -i input_dir\n");
		printf("It doesn't matter if the flags are in different order, as long as all are present and the respective argument follows after each flag\n");
		return 1;
	}
	int i;
	int numMonitors, bufferSize, sizeOfBloom;
	char *input_dir = malloc(255*sizeof(char));
	char *bufferSizeArg;
  	for(i = 1; i < 9; i += 2){
		if(strlen(argv[i]) != 2){
			printf("Invalid flag %s\n", argv[i]);
			return 1;
		}
		switch(argv[i][1]){
		case 'm': numMonitors = atoi(argv[i+1]);
				break;
		case 'b': bufferSize = atoi(argv[i+1]);
					break;
		case 's': sizeOfBloom = atoi(argv[i+1]);
				break;
		case 'i': strcpy(input_dir, argv[i+1]);
				break;
		default: printf("Invalid flag %s\n", argv[i]);
				return 1;
		}    
	}
	char *pipe_name = malloc(11*sizeof(char));
	int *read_file_descs = malloc(numMonitors*sizeof(int));
	for(i = 1; i <= numMonitors; i++){
		// Making the pipes 
		// Then, each child that will be exec'd
		// will receive the number of the pair of pipes
		// to open as an argument
		sprintf(pipe_name, "pipes/%dr", i);
		if(mkfifo(pipe_name, 0666) < 0){
			fprintf(stderr, "errno: %d\n", errno);
			perror("mkfifo");
		}
		// printf("About to make pipe for reading\n");
		read_file_descs[i-1] = open(pipe_name, O_RDONLY | O_NONBLOCK);
		if(read_file_descs[i-1] < 0){
			perror("open read pipe");
		}
		sprintf(pipe_name, "pipes/%dw", i);
		if(mkfifo(pipe_name, 0666) < 0){
			fprintf(stderr, "errno: %d\n", errno);
			perror("mkfifo");
		}
	}
	pid_t pid;
	// Creating monitor processes and passing them their arguments
	// Their id number, that will be used for quick reference to one after
	// we have queried its bloom filter and we wish to query its skiplist,
	// the bufferSizeArgument, and the pipes for communication.
	// readPipe is the pipe from which the parent reads and the monitor writes to.
	// writePipe is the pipe for the opposite direction of communication.
  	pid_t *children_pids = malloc(numMonitors*sizeof(int));
	char *path = "monitorProcess"; 
	char *readPipe = malloc(11*sizeof(char));
	char *writePipe = malloc(11*sizeof(char));
	for(i = 1; i <= numMonitors; i++){
		if((pid = fork()) < 0){
			perror("fork");
			return 1;
		}
		if(pid == 0){
			sprintf(readPipe, "pipes/%dr", i);
			sprintf(writePipe, "pipes/%dw", i);
			if(execlp(path, path, readPipe, writePipe, NULL) < 0){
				perror("execlp");
				return 1;
			}
		}else{
      			children_pids[i-1] = pid;
    		}
	}
	free(readPipe);
	free(writePipe);
	int *write_file_descs = malloc(numMonitors*sizeof(int));
	char *pipeReadBuffer = malloc(bufferSize*sizeof(char));
	char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
	int charsCopied;
	char countryLength = 0;
	// Opening pipes created from monitors where parent will write
	// the countries each monitor will process.
	for(i = 1; i <= numMonitors; i++){
		sprintf(pipe_name, "pipes/%dw", i);
		write_file_descs[i-1] = open(pipe_name, O_WRONLY);
		if(write_file_descs[i-1] < 0){
			perror("open write pipe");
		}else{
			passCommandLineArgs(read_file_descs[i-1], write_file_descs[i-1], bufferSize, sizeOfBloom, input_dir);
		}
	}
	struct dirent **alphabeticOrder;
	int subdirCount;
	subdirCount = scandir(input_dir, &alphabeticOrder, NULL, alphasort);
	if(subdirCount == -1){
		perror("scandir");
	}
	int roundRobin = 0;
	fd_set rd;
	// distributing country names in alphabetical round robin fashion. 
	for(i = 0; i < subdirCount; i++){
		if(strcmp(alphabeticOrder[i]->d_name, ".") == 0 || strcmp(alphabeticOrder[i]->d_name, "..") == 0){
			continue;
		}
		FD_ZERO(&rd);
		FD_SET(read_file_descs[roundRobin], &rd);
		if(select(read_file_descs[roundRobin] + 1, &rd, NULL, NULL, NULL) == -1){
			perror("select for child ok confirmation");
		}else{
			if(read(read_file_descs[roundRobin], pipeReadBuffer, bufferSize) < 0){
				perror("read for child ok confirmation");
			}
			charsCopied = 0;
			countryLength = strlen(alphabeticOrder[i]->d_name);
			if(write(write_file_descs[roundRobin], &countryLength, sizeof(char)) < 0){
				perror("write");
			}else{
				while(charsCopied < countryLength){
					strncpy(pipeWriteBuffer, alphabeticOrder[i]->d_name + charsCopied, bufferSize);
					if(write(write_file_descs[roundRobin], pipeWriteBuffer, bufferSize) < 0){
						perror("write");
					}
					charsCopied += bufferSize;
				}
			}
		}
		roundRobin++;
		roundRobin = roundRobin % numMonitors;
	}
	char *endOfMessage = "END";
	for(i = 1; i <= numMonitors; i++){
		FD_ZERO(&rd);
		FD_SET(read_file_descs[i-1], &rd);
		if(select(read_file_descs[i-1] + 1, &rd, NULL, NULL, NULL) == -1){
			perror("select for last child ok confirmation");
		}else{	
			if(read(read_file_descs[i-1], pipeReadBuffer, bufferSize) < 0){
				perror("read for last child ok confirmation");
			}
			charsCopied = 0;
			countryLength = strlen(endOfMessage);
			if(write(write_file_descs[i-1], &countryLength, sizeof(char)) < 0){
				perror("write");
			}else{
				while(charsCopied < countryLength){
					strncpy(pipeWriteBuffer, endOfMessage + charsCopied, bufferSize);
					if(write(write_file_descs[i-1], pipeWriteBuffer, bufferSize) < 0){
						perror("write");
					}
					charsCopied += bufferSize;
				}
			}
		}
	}
	// Creating lookup table for countries.
	// Each virus will hold multiple bloomfilters and we must know which one
	// is about the country in question. Each country will hold a payload, the result
	// of the operation (index in alphabeticOrder) % numMonitors + 1. This is the
	// number of the bloom filter in question.
	hashMap *country_map;
	create_map(&country_map, 43, Country_List);
	int payload;
	roundRobin = 0;
	for(i = 0; i < subdirCount; i++){
		if(strcmp(alphabeticOrder[i]->d_name, ".") == 0 || strcmp(alphabeticOrder[i]->d_name, "..") == 0){
			free(alphabeticOrder[i]);
			continue;
		}
		payload = roundRobin % numMonitors;
		roundRobin++;
		insert(country_map, alphabeticOrder[i]->d_name, (Country *) create_country(alphabeticOrder[i]->d_name, payload)); 
		free(alphabeticOrder[i]);
	}
	free(alphabeticOrder);
	// Creating hashMap of sets of bloomfilters.
	hashMap *setOfBFs_map;
	create_map(&setOfBFs_map, 3, BFLookup_List);
	setofbloomfilters *curr_set;
	// This array shows from which monitors we have NOT received
	// their bloomfilters yet. When we receive the bloom filters of one monitor,
	// the respective element in this array takes the value zero.
	int *read_bloom_descs = malloc(numMonitors*sizeof(int));
	FD_ZERO(&rd);
	for(i = 0; i < numMonitors; i++){
		FD_SET(read_file_descs[i], &rd);
		read_bloom_descs[i] = 1;
	}
	int max = read_file_descs[i-1] + 1;
	char dataLength, charactersRead, charactersParsed;
	char *virusName = calloc(255, sizeof(char));
	for(i = 0; i < numMonitors; ){
		// printf("Iteration of select/read duo...\n");
		if(select(max, &rd, NULL, NULL, NULL) == -1){
			perror("select virus names");
		}else{
			for(int k = 0; k < numMonitors; k++){
				if(read_bloom_descs[k] == 1 && FD_ISSET(read_file_descs[k], &rd)){
					// We will now read from the monitor process no. i
					// printf("About to receive faux bloom filters from monitor %d\n", k);
					// Reading...
					receiveBloomFiltersFromChild(setOfBFs_map, read_file_descs[k], write_file_descs[k], k, bufferSize, numMonitors, sizeOfBloom);
					i++;
					// After reading, we must reinitialize the set of file descs
					// that we expect 'traffic' from.
					read_bloom_descs[k] = 0;
				}
			}
			max = 0;
			FD_ZERO(&rd);
			for(int j = 0; j < numMonitors; j++){
				if(read_bloom_descs[j] == 1){
					FD_SET(read_file_descs[j], &rd);
					if(read_file_descs[j] > max){
						max = read_file_descs[j];
					}
				}
			}
			max++;			
		}
	}
  printf("Received data from children processes.\n");
  // Since we assume that SIGINT/SIGQUIT will be sent to a child process after it has been sent the file directories,
  // and it has processed the bloomfilters, the first of periodic checks for a child that has quit will take place here.
  hasChildExited(country_map, setOfBFs_map, &(children_pids), read_file_descs, write_file_descs, numMonitors, bufferSize, sizeOfBloom, input_dir);
  
  size_t command_length = 1024, actual_length;
  char *command = malloc(command_length*sizeof(char));
  char *command_name, *rest;
  char *dateOfTravel = malloc(12*sizeof(char));
  char *citizenID = malloc(5*sizeof(char));
  char *countryName = malloc(255*sizeof(char));
  char charsToWrite;
  Country *curr_country;
  fd_set standin;
  FD_SET(0, &standin);
  printf("Ready to accept commands.\n");
  while(1){
	// Checking if the SIGCHLD signal was received during the previous operation with another monitor.
	hasChildExited(country_map, setOfBFs_map, &(children_pids), read_file_descs, write_file_descs, numMonitors, bufferSize, sizeOfBloom, input_dir);    
	if(select(1, &standin, NULL, NULL, NULL) < 1 && errno == EINTR){
		// Checking if the SIGCHLD signal was received while waiting for input from keyboard. 
		hasChildExited(country_map, setOfBFs_map, &(children_pids), read_file_descs, write_file_descs, numMonitors, bufferSize, sizeOfBloom, input_dir);    
    }else{
		actual_length = getline(&command, &command_length, stdin);
		command_name = strtok_r(command, " ", &rest);
		if(strcmp(command_name, "/travelRequest") == 0){
			if(sscanf(rest, "%s %s %s %s", citizenID, dateOfTravel, countryName, virusName) == 4){
				if(dateFormatValidity(dateOfTravel) == -1){
					printf("Invalid travel date format. Try again.\n");
					continue;
				}
				travelRequest(setOfBFs_map, country_map, citizenID, dateOfTravel, countryName, virusName, bufferSize, read_file_descs, write_file_descs, &reqs);
			}else{
				printf("Bad arguments to /travelRequest. Try again.\n");
			}
		}else if(strcmp(command_name, "/exit\n") == 0){
			act.sa_handler=SIG_DFL;
			sigaction(SIGCHLD, &act, NULL);
			for(i = 0; i < numMonitors; i++){
				kill(children_pids[i], 9);
			}
			// waiting for children to exit...
			// printf("About to wait for my kids...\n");
			for(i = 1; i <= numMonitors; i++){
				printf("Waiting for kid no %d\n", i);
				if(wait(NULL) == -1){
				perror("wait");
				return 1;
				}
			}
			break;
		}else{
			printf("Unknown command. Try again.\n");
		}
	}
  }
	pid_t mypid = getpid();
	char *logfile = malloc(20*sizeof(char));
	sprintf(logfile, "log_file.%d", mypid);
	FILE *log = fopen(logfile, "w");
	assert(log != NULL);
	printSubdirectoryNames(country_map, log);
	fprintf(log, "TOTAL TRAVEL REQUESTS %d\nACCEPTED %d\nREJECTED %d\n", reqs.total, reqs.accepted, reqs.rejected);
	assert(fclose(log) == 0);
	free(logfile);
	free(command);
	free(dateOfTravel);
	free(citizenID);
	free(countryName);
	free(read_bloom_descs);
	free(virusName);
  	free(children_pids);
  	// Closing and deleting all pipes
	for(i = 1; i <= numMonitors; i++){
		close(read_file_descs[i-1]);
		sprintf(pipe_name, "pipes/%dr", i);
		if(unlink(pipe_name) < 0){
			perror("unlink");
		}
		close(write_file_descs[i-1]);
		sprintf(pipe_name, "pipes/%dw", i);
		if(unlink(pipe_name) < 0){
			perror("unlink");
		}
  	}
	destroy_map(&country_map);
	destroy_map(&setOfBFs_map);
	free(pipeWriteBuffer);
	free(pipeReadBuffer);
	free(read_file_descs);
	free(write_file_descs);
	free(pipe_name);
	free(input_dir);
}
