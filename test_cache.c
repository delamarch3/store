#include <stdio.h>

#include "cache.h"
#include "test.h"

#define TEST_STORE_FILE "test_cache.store"

void test_cache() {
    struct PageCache pc;
    pagec_init(TEST_STORE_FILE, &pc);
}
