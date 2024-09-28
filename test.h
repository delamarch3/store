#include <assert.h>
#include <stdio.h>

#define TEST(e)                                                                \
    if (!(e)) {                                                                \
        printf("FAIL: %s:%d %s %s\n", __ASSERT_FILE_NAME, __LINE__, __func__,  \
               #e);                                                            \
        remove(test_store_file);                                               \
        return false;                                                          \
    }

void test_cache();
void test_map();
