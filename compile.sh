#!/bin/bash
# -lXext for the Xdbe double buffer X extension
gcc -Ofast $1 -Denable_graphics_extension sl.c  -lm -lXext -lX11 -lpthread -o gfxbin
