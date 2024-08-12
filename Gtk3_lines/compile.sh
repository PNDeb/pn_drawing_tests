#!/usr/bin/env sh

test -e drawing && rm drawing
gcc -Wall -Wextra `pkg-config --cflags gtk+-3.0` -o drawing gtk3_w_lines.c `pkg-config --libs gtk+-3.0` -lm
