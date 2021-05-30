#!/bin/bash

if [[ ! -e ./convertbin ]]
then
 echo "'convertbin' doesn't exist, please run compile.sh first"
 exit
fi

if [[ ! $# -eq 1 ]]
then
 echo "This script builds and runs a johnsonscript program via the johnsonscript to C transcompiler"
 echo "It accepts only one argument: program text or a file path to program text"
 exit
fi

if [[ -e a.out ]]
then
 rm a.out
fi

./convertbin "$1" > TEMP_FILE.c
gcc TEMP_FILE.c -lm -lXext -lX11 -lpthread
rm TEMP_FILE.c
if [[ -e a.out ]]
then
 ./a.out
else
 echo "Compilation failed";
fi
