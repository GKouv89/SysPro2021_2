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
  echo No reading rights for inputFile
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
  if [[ ${countries[*]} =~ (^|[[:space:]])"${words[3]}"($|[[:space:]]) ]] # Looking for exact match in array
  then
    continue
  else
    countries+=("${words[3]}")
    directoryName="$2/${words[3]}" 
    mkdir $directoryName    
    for((j = 1; j <= $3; j++))
    do
      fileName="$directoryName/${words[3]}-$j.txt"
      touch $fileName
    done
    roundRobin["${words[3]}"]+=0
  fi
done

exec < $1
while read line
do
  readarray -d ' ' -t words <<< "$line"
  fileNum=roundRobin["${words[3]}"]
  fileNum=$(( $fileNum + 1 ))
  fileName="$2/${words[3]}/${words[3]}-$fileNum.txt"
  echo About to print to $fileName
  echo $line >> "$fileName"
  roundRobin["${words[3]}"]=$(( roundRobin["${words[3]}"] + 1 ));
  roundRobin["${words[3]}"]=$(( roundRobin["${words[3]}"] % $3 ))
done
