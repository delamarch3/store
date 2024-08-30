#include <assert.h>
#include <stdio.h>

#include "cache.h"
#include "test.h"

#define TEST_STORE_FILE "test_cache.store"

static void test_cache_single_page();

void test_cache() { test_cache_single_page(); }

static void test_cache_single_page() {
    typedef struct TestPage TestPage;
    struct TestPage {
        size_t len;
        int values[];
    };

    PageCache pc = {0};
    cache_init(TEST_STORE_FILE, &pc);

    Page *page = NULL;
    assert(cache_new_page(&pc, &page));
    assert(page->pid == FREE_LIST_PAGE_ID + 1);

    // Ensure a fetch of a cached page returns the same slot
    char *data = page->data;
    {
        Page *page = NULL;
        pageid_t pid = FREE_LIST_PAGE_ID + 1;

        assert(cache_fetch_page(&pc, pid, &page));
        assert(page->pid == FREE_LIST_PAGE_ID + 1);
        assert(page->data == data);

        // Ensure the the pin count is correct
        LRUEntry *entry = lru_find_entry(&pc.lru, 255);
        assert(entry != NULL);
        assert(entry->evictable == false);

        // Ensure written data can be read by other pins
        TestPage *tp = (TestPage *)page->data;
        tp->values[tp->len++] = -1;
        tp->values[tp->len++] = 0;
        tp->values[tp->len++] = 1;

        cache_unpin(&pc, page);
    }

    // Ensure the lru entry is correct
    LRUEntry *entry = lru_find_entry(&pc.lru, 255);
    assert(entry != NULL);
    assert(entry->evictable == false);

    // Ensure written data can be read by other pins
    TestPage *tp = (TestPage *)page->data;
    assert(tp->len == 3);
    assert(tp->values[0] == -1);
    assert(tp->values[1] == 0);
    assert(tp->values[2] == 1);

    // Ensure slot is marked evictable
    cache_unpin(&pc, page);
    entry = lru_find_entry(&pc.lru, 255);
    assert(entry != NULL);
    assert(entry->evictable == true);

    // Ensure a page written to disk is read back the same
    cache_flush_page(&pc, page);
    cache_close(&pc);
    cache_init(TEST_STORE_FILE, &pc);

    page = NULL;
    pageid_t pid = FREE_LIST_PAGE_ID + 1;
    assert(cache_fetch_page(&pc, pid, &page));
    assert(page->pid == FREE_LIST_PAGE_ID + 1);

    tp = (TestPage *)page->data;
    assert(tp->len == 3);
    assert(tp->values[0] == -1);
    assert(tp->values[1] == 0);
    assert(tp->values[2] == 1);

    // Ensure the next allocated page is correct
    page = NULL;
    assert(cache_new_page(&pc, &page));
    assert(page->pid == FREE_LIST_PAGE_ID + 2);

    // TODO: failed asserts will skip test file removal
    // Remove test store
    remove(TEST_STORE_FILE);

    return;
}
