#include <assert.h>
#include <stdio.h>

#include "cache.h"
#include "test.h"

#define TEST_STORE_FILE "test_cache.store"

static void test_cache_single_page();

static LRUEntry *lru_find_entry(const LRU *lru, slotid_t sid) {
    for (size_t i = 0; i < lru->entries.len; i++) {
        LRUEntry *entry = &lru->entries.data[i];
        if (entry->sid == sid) {
            return entry;
        }
    }

    return NULL;
}

void test_cache() { test_cache_single_page(); }

static void test_cache_single_page() {
    struct TestPage {
        size_t len;
        int values[];
    };

    struct PageCache pc = {0};
    pagec_init(TEST_STORE_FILE, &pc);

    struct Pin pin = {0};
    assert(pagec_new_page(&pc, &pin));
    assert(pin.pid == FREE_LIST_PAGE_ID + 1);
    assert(pin.lru == &pc.lru);

    // Ensure a fetch of a cached page returns the same slot
    char *page = pin.page;
    slotid_t sid = pin.sid;
    {
        struct Pin pin = {0};
        pin.pid = FREE_LIST_PAGE_ID + 1;

        assert(pagec_fetch_page(&pc, &pin));
        assert(pin.pid == FREE_LIST_PAGE_ID + 1);
        assert(pin.sid == sid);
        assert(pin.page == page);

        // Ensure the the pin count is correct
        LRUEntry *entry = lru_find_entry(pin.lru, pin.sid);
        assert(entry != NULL);
        assert(entry->pins == 2);

        // Ensure written data can be read by other pins
        struct TestPage *tp = (struct TestPage *)pin.page;
        tp->values[tp->len++] = -1;
        tp->values[tp->len++] = 0;
        tp->values[tp->len++] = 1;

        lru_unpin(pin.lru, pin.sid);
    }

    // Ensure the pin count is correct
    LRUEntry *entry = lru_find_entry(pin.lru, pin.sid);
    assert(entry != NULL);
    assert(entry->pins == 1);

    // Ensure written data can be read by other pins
    struct TestPage *tp = (struct TestPage *)pin.page;
    assert(tp->len == 3);
    assert(tp->values[0] == -1);
    assert(tp->values[1] == 0);
    assert(tp->values[2] == 1);

    // Ensure a page written to disk is read back the same
    pagec_flush_page(&pc, &pin);
    pagec_free(&pc);
    pagec_init(TEST_STORE_FILE, &pc);

    pin = (struct Pin){0};
    pin.pid = FREE_LIST_PAGE_ID + 1;
    assert(pagec_fetch_page(&pc, &pin));
    assert(pin.pid == FREE_LIST_PAGE_ID + 1);

    tp = (struct TestPage *)pin.page;
    assert(tp->len == 3);
    assert(tp->values[0] == -1);
    assert(tp->values[1] == 0);
    assert(tp->values[2] == 1);

    // Ensure the next allocated page is correct
    pin = (struct Pin){0};
    assert(pagec_new_page(&pc, &pin));
    assert(pin.pid == FREE_LIST_PAGE_ID + 2);
    assert(pin.sid == sid - 1);
    assert(pin.lru == &pc.lru);

    // Remove test store
    remove(TEST_STORE_FILE);

    return;
}
