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

#include "../include/setofbfs.h"
#include "../include/country.h"
#include "../include/hashmap.h"

extern int errno;

int main(int argc, char *argv[]){
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
  	int *children_pids = malloc(numMonitors*sizeof(int));
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
			if(write(write_file_descs[i-1], &bufferSize, sizeof(int)) < 0){
				perror("write bufferSize");
			}else{
				// Waiting for confirmation from the child for bufferSize
				while(read(read_file_descs[i-1], pipeReadBuffer, bufferSize) < 0);
				// Letting child know size of bloomfilters
				if(write(write_file_descs[i-1], &sizeOfBloom, sizeof(int)) < 0){
					perror("write sizeOfBloom length");
				}	
				// Waiting for confirmation from the child for sizeOfBloom
				while(read(read_file_descs[i-1], pipeReadBuffer, bufferSize) < 0);
				// Letting monitors know input_dir's length and then name,
				// so they have the full path to their subdirectories.
				countryLength = strlen(input_dir); // Not an actual country yet.
				if(write(write_file_descs[i-1], &countryLength, sizeof(char)) < 0){
					perror("write input_dir length");
				}else{
					charsCopied = 0;
					while(charsCopied < countryLength){
						strncpy(pipeWriteBuffer, input_dir + charsCopied, bufferSize);
						if(write(write_file_descs[i-1], pipeWriteBuffer, bufferSize) < 0){
							perror("write input_dir chunk");
						}
						charsCopied += bufferSize;
					}
				}
			}
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
					while(1){
						if(read(read_file_descs[k], &dataLength, sizeof(char)) < 0){
							// perror("read virus name length");
							continue;
						}else{
							// printf("Virus name length: %d\n", dataLength);
							charactersParsed = 0;
							while(charactersParsed < dataLength){
								if((charactersRead = read(read_file_descs[k], pipeReadBuffer, bufferSize)) < 0){
									// This means that the monitor isn't done writing the chunk to the pipe,
									// but it will be ready shortly, so we just move on to the next rep.
									continue;
								}else{
									strncat(virusName, pipeReadBuffer, charactersRead);
									charactersParsed+=charactersRead;
								}		
							}
							if(strcmp(virusName, "END") == 0){
								// printf("Done receiving from monitor no %d\n", k);
								i++;
								memset(virusName, 0, 255*sizeof(char));
								break;
							}else{
								// printf("Received faux bloom filter for Virus %s from monitor %d\n", virusName, k);
								curr_set = (setofbloomfilters *) find_node(setOfBFs_map, virusName);
								if(curr_set == NULL){
									create_setOfBFs(&curr_set, virusName, numMonitors, sizeOfBloom);
									add_BFtoSet(curr_set, k);
									insert(setOfBFs_map, virusName, curr_set);
								}else{
									add_BFtoSet(curr_set, k);
								}
								if(write(write_file_descs[k], "1", sizeof(char)) < 0){
									perror("write confirmation after receiving virus name");
								}
								read_BF(curr_set, read_file_descs[k], write_file_descs[k], k, bufferSize);
								memset(virusName, 0, 255*sizeof(char));
							}
						}
					}
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
  size_t command_length = 1024, actual_length;
  char *command = malloc(command_length*sizeof(char));
  char *request = malloc(255*sizeof(char));
  char request_length;
  char *command_name, *rest;
  char *answer = malloc(4*sizeof(char));
  char *date = malloc(11*sizeof(char));
  char *citizenID = malloc(5*sizeof(char));
  char *countryName = malloc(255*sizeof(char));
  char charsToWrite;
  Country *curr_country;
  while(1){
    actual_length = getline(&command, &command_length, stdin);
    command_name = strtok_r(command, " ", &rest);
    if(strcmp(command_name, "/travelRequest") == 0){
      if(sscanf(rest, "%s %s %s", citizenID, countryName, virusName) == 3){
        curr_set = (setofbloomfilters *) find_node(setOfBFs_map, virusName);
		    if(curr_set == NULL){
          printf("No such virus: %s\n", virusName);
          continue;
        }else{
          // Find which monitor handled the country's subdirectory
          curr_country = (Country *) find_node(country_map, countryName);
          if(curr_country == NULL){
            printf("No such country: %s\n", countryName);
            continue;
          }
          if(lookup_bf_vaccination(curr_set, curr_country->index, citizenID) == 1){
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
            // Reading actual request, byte by byte.
            charsCopied = 0;
            memset(request, 0, 255*sizeof(char));
            while(charsCopied < request_length){
              if((charactersRead = read(read_file_descs[curr_country->index], pipeReadBuffer, bufferSize)) < 0){
                continue;
              }else{
                strcat(request, pipeReadBuffer);
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
              sscanf(request, "%s %s", answer, date);
              printf("VACCINATED ON %s\n", date);
            }
          }else{
            printf("REQUESTED REJECTED - YOU ARE NOT VACCINATED\n");
          }
        } 
      }else{
        printf("Bad arguments to /travelRequest. Try again.\n");
      }
    }else if(strcmp(command_name, "/exit\n") == 0){
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
	free(command);
  free(answer);
  free(date);
  free(request);
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
