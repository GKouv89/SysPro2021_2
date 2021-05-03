#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/bucketlist.h"
#include "../include/country.h"
#include "../include/citizen.h"
#include "../include/virus.h"
#include "../include/setofbfs.h"

void create_bucketList(bucketList **bl, typeOfList type){
  (*bl) = malloc(sizeof(bucketList));
  (*bl)->front = (*bl)->rear = NULL;
  (*bl)->type = type;
}

void insert_bucketNode(bucketList *bl, void *content){
  bucketNode *new_node = malloc(sizeof(bucketNode));
  new_node->content = content;
  new_node->next = NULL;
  if(bl->front == NULL && bl->rear == NULL){
    bl->front = new_node;
    bl->rear = new_node;
  }else{
    bl->rear->next = new_node;
    bl->rear = new_node;
  }
}

//////////////////////////////////////////////////////////
// The search function called from the find_node        //
// function of the hashmap module. According to the type//
// of bucketlist (and therefore, the type of hashmap)   //
// the appropriate comparison function is called.       //
//////////////////////////////////////////////////////////

void* search_bucketList(bucketList *bl, char *str){
  bucketNode *temp = bl->front;
  while(temp){
    if(bl->type == Country_List){
      if(isEqual_country(temp->content, str)){
        return temp->content;
      }
    }else if(bl->type == Virus_List){
      if(isEqual_virus(temp->content, str)){
        return temp->content;
      }
    }else if(bl->type == Citizen_List){
      if(isEqual_citizen(temp->content, str)){
        return temp->content;
      }
    }else{
      if(isEqual_setOfBFs(temp->content, str)){
        return temp->content;
      }
    }
    temp = temp->next;
  }
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// This is called when the monitor process is about to send the bloomfilters //
// after the first reading of the files.                                     //
///////////////////////////////////////////////////////////////////////////////

void send_virus_Bloomfilters(bucketList *bl, int readfd, int writefd, int bufferSize){
   if(bl->type == Virus_List){
      bucketNode *temp = bl->front;
      unsigned char virusNameLength, charsCopied, charsToWrite;
      char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
      char *pipeReadBuffer = malloc(bufferSize*sizeof(char));
      bloomFilter *bf;
      int bfSize;
      int bytesTransferred;
      while(temp){
        // First, send virus name length.
        virusNameLength = strlen(((Virus *)temp->content)->name);
        if(write(writefd, &virusNameLength, sizeof(char)) < 0){
            perror("write virus name length");
        }else{
          charsCopied = 0;
          while(charsCopied < virusNameLength){
            strncpy(pipeWriteBuffer, ((Virus *)temp->content)->name + charsCopied, bufferSize);
            if(write(writefd, pipeWriteBuffer, bufferSize) < 0){
              perror("write virus name chunk");
            }
            charsCopied += bufferSize;
          }
          // Waiting for confirmation that the virus name was received in its entirety.
          while(read(readfd, pipeReadBuffer, bufferSize) < 0);
          bf = ((Virus *)temp->content)->virusBF;
          bytesTransferred = 0;
          bfSize = (bf->size)/8;
          // At first, just COPYING the filter chunk by chunk to the buffer
          while(bytesTransferred < bfSize){
            if(bfSize - bytesTransferred < bufferSize){
              charsToWrite = bfSize - bytesTransferred;
            }else{
              charsToWrite = bufferSize;
            }
            memcpy(pipeWriteBuffer, bf->filter + bytesTransferred, charsToWrite*sizeof(char));
            bytesTransferred += charsToWrite;
            // printf("bytesTransferred: %d\n", bytesTransferred);
            if(write(writefd, pipeWriteBuffer, charsToWrite*sizeof(char)) < 0){
              perror("write bf chunk\n");
            }
          }
          printf("BLOOM COPIED\n");
          while(read(readfd, pipeReadBuffer, bufferSize) < 0);
        }
        temp = temp->next;
      }
      free(pipeWriteBuffer);
      free(pipeReadBuffer);
   } 
}

////////////////////////////////////////////////////////////////////////////////////////////////
// This function is called after one of the children was killed and the parent                //
// spawns a new child in its place. It is checked which country was processed by the old child//
// and those are communicate to the new child.                                                //
////////////////////////////////////////////////////////////////////////////////////////////////

void sendCountriesToChild(bucketList *bl, int readfd, int writefd, int bufferSize, int monitorIndex){
  char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
  char *pipeReadBuffer = malloc(bufferSize*sizeof(char));  
  char *countryName = malloc(255*sizeof(char));
  char countryLength, charsCopied, charsToWrite;
  if(bl->type == Country_List){
    bucketNode *temp = bl->front;
    while(temp){
      // Waiting for confirmation that the country name was received in its entirety.
      // First confirmation we wait for was for reception of input directory's name.
      while(read(readfd, pipeReadBuffer, bufferSize) < 0);
      if(((Country *)temp->content)->index == monitorIndex){
        // First, send virus name length.
        countryLength = strlen(((Country *)temp->content)->name);
        if(write(writefd, &countryLength, sizeof(char)) < 0){
            perror("write country name length");
        }else{
          charsCopied = 0;
          while(charsCopied < countryLength){
            if(countryLength - charsCopied < bufferSize){
              charsToWrite = countryLength - charsCopied;
            }else{
              charsToWrite = bufferSize;
            }
            strncpy(pipeWriteBuffer, ((Country *)temp->content)->name + charsCopied, charsToWrite);
            if(write(writefd, pipeWriteBuffer, charsToWrite) < 0){
              perror("write virus name chunk");
            }
            charsCopied += charsToWrite;
          }
        }
      }
      temp = temp->next;
    }
  }
  free(pipeWriteBuffer);
  free(pipeReadBuffer);
  free(countryName);
}


///////////////////////////////////////////////////////////////
// The command that is called when vaccineStatus is called   //
// without specifying the virus. Called for the list of every//
// hash bucket, for every node of the list (every virus)     //
// a search through the skiplist containing the vaccinated   //
// people for the virus occurs.                              //
///////////////////////////////////////////////////////////////

// void vacStatus_all(bucketList *bl, unsigned char *citizenID){
//   if(bl->type == Virus_List){
//     bucketNode *temp = bl->front;
//     listNode *res;
//     while(temp){
//       res = lookup_in_virus_vaccinated_for_list((Virus *)temp->content, atoi(citizenID));
//       if(res == NULL){
//         printf("%s NO\n", ((Virus *)temp->content)->name);
//       }else{
//         printf("%s YES %s\n", ((Virus *)temp->content)->name, res->vaccinationDate);
//       }
//       temp = temp->next;
//     }
//   }
// }

//////////////////////////////////////////////////////////////////////////////
// The command that is called when popStatus or popStatusByAge is called    //
// without specifying the country. Called for the list of every             //
// hash bucket of the countries hashmap, for every node of the list         //
// (every country) the function that finds the appropriate population counts//
// and ratios for the virus is called with said country as an argument.     //
// ageMode indicates whether popStatus or popStatusByAge is called.         //
// Depending on that, not only the appropriate printing function is called, //
// but the result of the skiplist population search is casted               //
// with the correct type.                                                   //
//////////////////////////////////////////////////////////////////////////////

// void popStatus_all(bucketList *bl, int ageMode, Virus *v, char *startingDate, char *endingDate){
//   if(bl->type == Country_List){
//     bucketNode *temp = bl->front;
//     while(temp){
//       void *vacced = skiplist_vac_status_country(v->vaccinated_for, 1, ageMode, (Country *)temp->content, startingDate, endingDate);
//       void *notVacced = skiplist_vac_status_country(v->not_vaccinated_for, 0, ageMode, (Country *)temp->content, startingDate, endingDate);
//       if(ageMode == 0){
//         print_vaccination_ratio((Country *)temp->content, (struct vaccinations *)vacced, (struct vaccinations *)notVacced);
//       }else{
//         print_vaccination_ratios_byAge((Country *)temp->content, (struct vaccinationsAgeGroup *)vacced, (struct vaccinationsAgeGroup *)notVacced);
//       }
//       temp = temp->next;
//     }
//   }
// }

void destroy_bucketList(bucketList **bl){
  bucketNode *temp = (*bl)->front;
  bucketNode *to_del;
  while(temp){
    to_del = temp;
    temp = temp->next;
    if((*bl)->type == Country_List){
      Country *to_destroy = (Country *)to_del->content;
      destroy_country(&to_destroy);
    }else if((*bl)->type == Virus_List){
      Virus *to_destroy = (Virus *)to_del->content;
      destroy_virus(&to_destroy);
    }else if((*bl)->type == Citizen_List){
      Citizen *to_destroy = (Citizen *)to_del->content;
      destroy_citizen(&to_destroy);
    }else{
      setofbloomfilters *to_destroy = (setofbloomfilters *)to_del->content;
      destroy_setOfBFs(&to_destroy);
    }
    free(to_del);
    to_del = NULL;
  }
  free(*bl);
  *bl = NULL;
}
