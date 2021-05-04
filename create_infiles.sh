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
  fi
done
