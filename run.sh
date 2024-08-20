#!/bin/bash

# -Wconversion -Wdouble-promotion -Wno-unused-parameter \
# -Wno-unused-function -Wno-sign-conversion \

clang main.c disk.c -o main \
    -pedantic -Wall -Wextra \
    -fsanitize=address,undefined \
    -g3 -std=c2x
./main
