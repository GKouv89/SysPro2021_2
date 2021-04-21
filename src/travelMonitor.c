#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int errno;

int main(int argc, char *argv[]){
  if(argc != 9){
    printf("Usage: ./travelMonitor –m numMonitors -b bufferSize -s sizeOfBloom -i input_dir\n");
    printf("It doesn't matter if the flags are in different order, as long as all are present and the respective argument follows after each flag\n");
    return 1;
  }
  int i;
  int numMonitors, bufferSize, sizeOfBloom;
  char *input_dir = malloc(255*sizeof(char));
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
  int *file_descriptors = malloc(2*numMonitors*sizeof(int));
  int file_desc_iter = 0;
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
      file_descriptors[file_desc_iter] = open(pipe_name, O_RDONLY | O_NONBLOCK);
      if(file_descriptors[file_desc_iter] < 0){
        perror("open");
      }
      file_desc_iter++;
      sprintf(pipe_name, "pipes/%dw", i);
      if(mkfifo(pipe_name, 0666) < 0){
        fprintf(stderr, "errno: %d\n", errno);
        perror("mkfifo");
      }
      file_descriptors[file_desc_iter] = open(pipe_name, O_WRONLY | O_NONBLOCK);
      if(file_descriptors[file_desc_iter] < 0){
        perror("open");
      }
      file_desc_iter++;
  }
  file_desc_iter = 0;
  for(i = 1; i <= numMonitors; i++){
    close(file_descriptors[file_desc_iter]);
    file_desc_iter++;
    sprintf(pipe_name, "pipes/%dr", i);
    if(unlink(pipe_name) < 0){
      perror("unlink");
    }
    close(file_descriptors[file_desc_iter]);
    sprintf(pipe_name, "pipes/%dw", i);
    if(unlink(pipe_name) < 0){
      perror("unlink");
    }
    file_desc_iter++;
  }
  free(file_descriptors);
  free(pipe_name);
  free(input_dir);
}