#include <assert.h>
#include <stdio.h>

#include "cache.h"
#include "test.h"

#define TEST_STORE_FILE "test_cache.store"

void test_cache() {
    struct PageCache pc = {0};
    pagec_init(TEST_STORE_FILE, &pc);

    struct Pin pin = {0};
    assert(pagec_new_page(&pc, &pin));

    assert(pin.pid == 2);

    return;
}
