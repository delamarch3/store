#!/bin/bash

# -Wconversion -Wdouble-promotion -Wno-unused-parameter \
# -Wno-unused-function -Wno-sign-conversion \

files=(
    main.c
    disk.c
    cache.c
)

clang ${files[@]} -o main \
    -pedantic -Wall -Wextra \
    -fsanitize=address,undefined \
    -g3 -std=c2x
./main
