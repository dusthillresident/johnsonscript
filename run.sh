#!/bin/bash

if [[ ! -e ./convertbin ]]
then
 echo "'convertbin' doesn't exist, please run compile.sh first"
 exit
fi

if [[ $# -lt 1 ]]
then
 echo "This is the Johnsonscript trans-compiler run script"
 echo "Usage:"
 echo "$0 [program text or path to a file containing program text]"
 exit
fi

if [[ -e a.out ]]
then
 rm a.out
fi

./convertbin "$1" > TEMP_FILE.c
#gcc -Ofast -c mylib.c
gcc TEMP_FILE.c -lm -lXext -lX11 -lpthread
rm TEMP_FILE.c
if [[ -e a.out ]]
then
 # bash really just sucks, I end up using johnsonscript to do stuff instead because it's just such a pain to work out how you're meant to do it in bash
 ./gfxbin '
 variable i;
 stringvar cmdstr;
 set i 1
 while < i _argc
  set cmdstr cat$ cmdstr " " chr$34 $i chr$34
  set i + i 1
 endwhile
 oscli cat$ "./a.out " cmdstr
 quit' "$@"
else
 echo "Compilation failed";
fi
