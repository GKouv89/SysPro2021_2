#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]){
	// Arguments passed through exec:
	// An ID number: it's the number of the monitor process
	// from the parent's perspective. Needed so the monitor
	// know which pipes/Xw to write, where X is said number

	// Open the appropriate pipe for current process to read and parent process to write.
	char *writePipeName = malloc(11*sizeof(char));
	char *readPipeName = malloc(11*sizeof(char));
	sprintf(readPipeName, "pipes/%dw", atoi(argv[1]));
	if(mkfifo(readPipeName, 0666) < 0){
        	fprintf(stderr, "errno: %d\n", errno);
        	perror("mkfifo");
      	}
	int readfd = open(readPipeName, O_RDONLY | O_NONBLOCK);
        // Open the appropriate pipe created by parent for them to read and current process to write.
	sprintf(writePipeName, "pipes/%dr", atoi(argv[1]));
	int writefd = open(writePipeName, O_WRONLY);
	// Create log file of current process and wait to receive country names from parent.
	pid_t mypid = getpid();
	char *logFileName = malloc(14*sizeof(char));
	sprintf(logFileName, "log_file.%d", mypid);
//	FILE *logfile = fopen(logFileName, "w");

//	fclose(logfile);
	close(writefd);
	close(readfd);
	free(logFileName);
	free(writePipeName);
	free(readPipeName);
}
