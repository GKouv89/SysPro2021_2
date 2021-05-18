#!/bin/bash

if [ $# -ne 3 ]
then
  echo Invalid number of arguments.
  exit 1
fi

if [ ! -e $1 ]
then
  echo File $1 does not exist.
  exit 1
elif [ ! -f $1 ]
then
  echo File $1 is not a regular file.
  exit 1
elif [ ! -r $1 ]
then
  echo No reading rights
fi

if [ -d $2 ]
then
  echo Directory already exists
  exit 1
else
  mkdir $2
fi

exec < $1

declare -a countries
# In this array, the roundRobin file index is kept for each country
declare -A roundRobin

while read line
do
  readarray -d ' ' -t words <<< "$line"
  # If we have gone over this country again, just move on
  if [[ ${countries[*]} =~ (^|[[:space:]])"${words[3]}"($|[[:space:]]) ]] # Looking for exact match in array
  then
    continue
  else
    # If we haven't, we add it to the lookup array
    # create the directory and all the necessary files
    countries+=("${words[3]}")
    directoryName="$2/${words[3]}" 
    mkdir $directoryName    
    for((j = 1; j <= $3; j++))
    do
      fileName="$directoryName/${words[3]}-$j.txt"
      touch $fileName
    done
    # Starting to print from file 1 
    roundRobin["${words[3]}"]+=0
  fi
done

exec < $1
while read line
do
  readarray -d ' ' -t words <<< "$line"
  fileNum=roundRobin["${words[3]}"]
  # Get actual file name and not just the modulo
  # This is why the elements are initialized as 0 - to agree with the result of
  # the operation number % numFilesPerDirectory
  # But the file names follow 'normal', starting-from-1 enumeration
  fileNum=$(( $fileNum + 1 ))
  fileName="$2/${words[3]}/${words[3]}-$fileNum.txt"
  echo $line >> "$fileName"
  # Which will be the next file to print in?
  # Will we have to return to the first one?
  roundRobin["${words[3]}"]=$(( roundRobin["${words[3]}"] + 1 ));
  roundRobin["${words[3]}"]=$(( roundRobin["${words[3]}"] % $3 ))
done
