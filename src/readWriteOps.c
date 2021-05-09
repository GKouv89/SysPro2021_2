#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


void read_content(char **info_buffer, char **reading_buffer, int readfd, int bufferSize){
    char contentLength;
    while(read(readfd, &contentLength, sizeof(char)) < 0);
    char charsRead, charsCopied = 0;
    char *pipeReadBuffer = malloc(bufferSize*sizeof(char));
    while(charsCopied < contentLength){
        if((charsRead = read(readfd, pipeReadBuffer, bufferSize)) < 0){
            continue;
        }else{
            strncpy(*info_buffer + charsCopied, pipeReadBuffer, charsRead);
            charsCopied += charsRead;
        }
    }
    free(pipeReadBuffer);
}

void write_content(char *info_buffer, char **writing_buffer, int writefd, int bufferSize){
    char contentLength = strlen(info_buffer);
    if(write(writefd, &contentLength, sizeof(char)) < 0){
        perror("write content length");
    }else{
        char charsWritten = 0, charsToWrite;
        while(charsWritten < contentLength){
            if(contentLength - charsWritten < bufferSize){
                charsToWrite = contentLength - charsWritten;
            }else{
                charsToWrite = bufferSize;
            }
            strncpy(*writing_buffer, info_buffer + charsWritten, charsToWrite);
            if(write(writefd, *writing_buffer, charsToWrite*sizeof(char)) < 0){
                perror("write content chunk");
            }else{
                charsWritten += charsToWrite;
            }      
        }
    }
}