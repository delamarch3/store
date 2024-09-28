#!/bin/bash

# -Wconversion -Wdouble-promotion -Wno-unused-parameter \
# -Wno-unused-function -Wno-sign-conversion \

set -e

files=(
    disk.c
    cache.c
    map.c
)

test_files=(
    test_cache.c
    test_map.c
)

if [ $1 = 'test' ]
then
    clang test.c ${files[@]} ${test_files[@]} -o test \
        -pedantic -Wall -Wextra \
        -fsanitize=address,undefined \
        -g3 -std=c2x
    ./test
    exit 0
fi

clang main.c ${files[@]} -o main \
    -pedantic -Wall -Wextra \
    -fsanitize=address,undefined \
    -g3 -std=c2x
./main
