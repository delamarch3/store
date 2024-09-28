#include "cache.h"
#include "map.h"
#include "test.h"

static bool test_map_insert_and_get();
static bool test_map_split();

void test_map() {
    test_map_insert_and_get();
    test_map_split();
}

static bool test_map_insert_and_get() {
    char *test_store_file = "test_map_insert_and_get.store";

    PageCache pc = {0};
    cache_init(test_store_file, &pc);

    Map map = {0};
    map.pc = &pc;

    TEST(map_insert(&map, "k1", 2, "v1", 2));
    TEST(map_insert(&map, "k22", 3, "v22", 3));
    TEST(map_insert(&map, "k3", 2, "v3", 2));

    char *value = NULL;
    size_t vlen = 0;
    TEST(map_get(&map, "k3", 2, &value, &vlen));

    cache_close(&pc);

    return true;
}

static bool test_map_split() {
    //
    return true;
}
