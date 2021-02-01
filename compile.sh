#!/bin/bash
# -lXext for the Xdbe double buffer X extension
echo "// DO NOT EDIT THIS, instead edit tokenslist_src.c" > tokenslist.c
if [[ -e ./gfxbin ]]
then 
 cat tokenslist_src.c | ./gfxbin PENIStool.johnson >> tokenslist.c
fi
gcc -Ofast $1 -Denable_graphics_extension sl.c  -lm -lXext -lX11 -lpthread -o gfxbin
