#include <assert.h>
#include <stdio.h>

#include "cache.h"
#include "test.h"

#define STRINGIFY(e) #e
#define TO_STRING(e) STRINGIFY(e)
#define TEST_STORE_FILE(ext) (__func__ TO_STRING(ext))

#define TEST(e)                                                                \
    if (!(e)) {                                                                \
        printf("FAIL: %s:%d %s %s\n", __ASSERT_FILE_NAME, __LINE__, __func__,  \
               #e);                                                            \
        remove(test_store_file);                                               \
        return false;                                                          \
    }

static bool test_cache_single_page();

void test_cache() { test_cache_single_page(); }

static bool test_cache_single_page() {
    char *test_store_file = "test_cache_single_page.store";

    typedef struct TestPage TestPage;
    struct TestPage {
        size_t len;
        int values[];
    };

    PageCache pc = {0};
    cache_init(test_store_file, &pc);

    Page *page = NULL;
    TEST(cache_new_page(&pc, &page));
    TEST(page->pid == FREE_LIST_PAGE_ID + 1);

    // Ensure a fetch of a cached page returns the same slot
    char *data = page->data;
    {
        Page *page = NULL;
        pageid_t pid = FREE_LIST_PAGE_ID + 1;

        TEST(cache_fetch_page(&pc, pid, &page));
        TEST(page->pid == FREE_LIST_PAGE_ID + 1);
        TEST(page->data == data);

        // Ensure the the pin count is correct
        LRUEntry *entry = lru_find_entry(&pc.lru, 255);
        TEST(entry != NULL);
        TEST(entry->evictable == false);

        // Ensure written data can be read by other pins
        TestPage *tp = (TestPage *)page->data;
        tp->values[tp->len++] = -1;
        tp->values[tp->len++] = 0;
        tp->values[tp->len++] = 1;

        cache_unpin(&pc, page);
    }

    // Ensure the lru entry is correct
    LRUEntry *entry = lru_find_entry(&pc.lru, 255);
    TEST(entry != NULL);
    TEST(entry->evictable == false);

    // Ensure written data can be read by other pins
    TestPage *tp = (TestPage *)page->data;
    TEST(tp->len == 3);
    TEST(tp->values[0] == -1);
    TEST(tp->values[1] == 0);
    TEST(tp->values[2] == 1);

    // Ensure slot is marked evictable
    cache_unpin(&pc, page);
    entry = lru_find_entry(&pc.lru, 255);
    TEST(entry != NULL);
    TEST(entry->evictable == true);

    // Ensure a page written to disk is read back the same
    cache_flush_page(&pc, page);
    cache_close(&pc);
    cache_init(test_store_file, &pc);

    page = NULL;
    pageid_t pid = FREE_LIST_PAGE_ID + 1;
    TEST(cache_fetch_page(&pc, pid, &page));
    TEST(page->pid == FREE_LIST_PAGE_ID + 1);

    tp = (TestPage *)page->data;
    TEST(tp->len == 3);
    TEST(tp->values[0] == -1);
    TEST(tp->values[1] == 0);
    TEST(tp->values[2] == 1);

    // Ensure the next allocated page is correct
    page = NULL;
    TEST(cache_new_page(&pc, &page));
    TEST(page->pid == FREE_LIST_PAGE_ID + 2);

    remove(test_store_file);
    return true;
}
