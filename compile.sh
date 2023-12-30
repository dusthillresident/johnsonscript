#!/bin/bash
# -lXext for the Xdbe double buffer X extension
echo "Checking for johnsonscript interpreter (you may see an error message)"
interpreterIsWorking=$(./gfxbin "print 1; return 0")
echo "Check complete"
if [[ $interpreterIsWorking ]]
then 
 echo "// DO NOT EDIT THIS, instead edit tokenslist_src.c" > tokenslist.c
 cat tokenslist_src.c | ./gfxbin PENIStool.johnson >> tokenslist.c
else
 cp tokenslist_bak.c tokenslist.c
fi
function CheckExist {
 if [[ -e "$1" ]]
 then
  echo "OK"
 else
  echo "Failed"
 fi
}
JohnsonscriptInterpreterExecutable=gfxbin
JohnsonscriptTranscompilerExecutable=convertbin
echo -n "Building johnsonscript interpreter (executable '"$JohnsonscriptInterpreterExecutable"'): "
gcc -O2  $1 -Denable_graphics_extension sl.c  -lm -lXext -lX11 -lpthread -o $JohnsonscriptInterpreterExecutable
CheckExist $JohnsonscriptInterpreterExecutable
echo -n "Building johnsonscript->C transcompiler (executable '"$JohnsonscriptTranscompilerExecutable"'): "
gcc $1 -Denable_graphics_extension transpiler.c  -lm -lXext -lX11 -lpthread -o convertbin
CheckExist $JohnsonscriptTranscompilerExecutable

