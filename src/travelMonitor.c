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
      printf("About to make pipe for reading\n");
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
    }
  }
  free(readPipe);
  free(writePipe);
  int *write_file_descs = malloc(numMonitors*sizeof(int));
  // Opening pipes created from monitors where parent will write
  // the countries each monitor will process.
  for(i = 1; i <= numMonitors; i++){
    sprintf(pipe_name, "pipes/%dw", i);
    printf("About to open pipe %s for writing\n", pipe_name);
    write_file_descs[i-1] = open(pipe_name, O_WRONLY);
    printf("Opened pipe %s for writing\n", pipe_name);
    if(write_file_descs[i-1] < 0){
      perror("open write pipe");
    }else{
      if(write(write_file_descs[i-1], &bufferSize, sizeof(int)) < 0){
        perror("write");
      }
    }
  }
/*char *pipeReadBuffer = malloc(bufferSize*sizeof(char));
  char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
  DIR *inputDirectory = opendir(input_dir);
  struct dirent *currentFolder = readdir(inputDirectory);
  int charsCopied, countryLength = 0;
  int roundRobin = 0;
  while(currentFolder != NULL){
    charsCopied = 0;
    countryLength = strlen(currentFolder->d_name) + 1;
    while(charsCopied < countryLength){
      strncpy(pipeWriteBuffer, currentFolder->d_name + charsCopied, bufferSize);
      if(write(write_file_descs[roundRobin], pipeWriteBuffer, bufferSize) < 0){
        perror("write");
        return 1;
      }
      charsCopied += bufferSize;
    }
    roundRobin++;
    roundRobin = roundRobin % numMonitors;
    currentFolder = readdir(inputDirectory);
  }
  closedir(inputDirectory);
  fprintf(stderr, "Country names distributed");
  char *endOfMessage = "END";
  for(i = 1; i <= numMonitors; i++){
    charsCopied = 0;
    countryLength = strlen(endOfMessage) + 1;
    while(charsCopied < countryLength){
      strncpy(pipeWriteBuffer, endOfMessage + charsCopied, bufferSize);
      if(write(write_file_descs[roundRobin], pipeWriteBuffer, bufferSize) < 0){
        perror("write");
        return 1;
      }
      charsCopied += bufferSize;
    }
  } */
  // waiting for children to exit...
  printf("About to wait for my kids...\n");
  for(i = 1; i <= numMonitors; i++){
    printf("Waiting for kid no %d\n", i);
    if(wait(NULL) == -1){
      perror("wait");
      return 1;
    }
  }
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
//  free(pipeWriteBuffer);
  free(read_file_descs);
  free(write_file_descs);
  free(pipe_name);
  free(input_dir);
}
