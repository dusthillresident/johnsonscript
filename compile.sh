#!/bin/bash
# -lXext for the Xdbe double buffer X extension
gcc -Ofast -Denable_graphics_extension sl.c  -lm -lXext -lX11 -lpthread -o gfxbin
