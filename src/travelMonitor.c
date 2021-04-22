#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
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
      read_file_descs[i-1] = open(pipe_name, O_RDONLY | O_NONBLOCK);
      if(read_file_descs[i-1] < 0){
        perror("open");
      }
  }
  pid_t pid;
  char *path = "monitorProcess";
  char id[3];
  for(i = 1; i <= numMonitors; i++){
    if((pid = fork()) < 0){
      perror("fork");
      return 1;
    }
    if(pid == 0){
      sprintf(id, "%d", i);
      if(execlp(path, path, id, NULL) < 0){
        perror("execlp");
        return 1;
      }
    }
  }
  int *write_file_descs = malloc(numMonitors*sizeof(int));
  for(i = 1; i <= numMonitors; i++){
    write_file_descs[i-1] = open(pipe_name, O_WRONLY);
    if(write_file_descs[i-1] < 0){
      perror("open");
    }
  }
  for(i = 1; i <= numMonitors; i++){
    if(wait(NULL) == -1){
      perror("wait");
      return 1;
    }
  }
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
  free(read_file_descs);
  free(write_file_descs);
  free(pipe_name);
  free(input_dir);
}
