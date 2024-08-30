#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "cache.h"
#include "vec.h"

VEC_IMPL(slotid_t)
VEC_IMPL(LRUEntry)
VEC_IMPL(PageSlot)

static bool _find_cache_page(const PageCache *pc, pageid_t pid, slotid_t *sid) {
    for (size_t i = 0; i < pc->ptable.len; i++) {
        if (pc->ptable.data[i].pid == pid) {
            *sid = pc->ptable.data[i].sid;

            return true;
        }
    }

    return false;
}

static void _insert_cache_page(PageCache *pc, pageid_t pid, slotid_t sid) {
    for (size_t i = 0; i < pc->ptable.len; i++) {
        if (pc->ptable.data[i].pid == pid) {
            pc->ptable.data[i].sid = sid;
            return;
        }
    }

    vec_push_PageSlot(&pc->ptable, (PageSlot){.pid = pid, .sid = sid});
}

// Attempt to find a free/evictable slot in the page cache to hold the page
// specified in the pin. Returns false if there is no free or evictable page
static bool _try_get_page(PageCache *pc, pageid_t pid, Page **page) {
    // Try to find a free page
    slotid_t sid = 0;
    if (!vec_pop_slotid_t(&pc->free, &sid) && !lru_evict(&pc->lru, &sid)) {
        // There is no free or evicatable page
        return false;
    }

    Page *cache_page = &pc->pages[sid];
    assert(cache_page->pins == 0);

    // Register entry into LRU
    lru_register_entry(&pc->lru, sid);
    lru_access(&pc->lru, sid);
    cache_page->pins = 1;

    // Write old page if dirty
    if (cache_page->dirty) {
        disk_write(&pc->dm, cache_page->pid, cache_page->data);
        cache_page->dirty = false;
    }

    // Read new page
    cache_page->pid = pid;
    disk_read(&pc->dm, cache_page->pid, cache_page->data);

    // Insert pid -> sid into page table
    _insert_cache_page(pc, cache_page->pid, sid);

    *page = cache_page;

    return true;
}

void cache_init(char *path, PageCache *pc) {
    disk_open(path, &pc->dm);

    lru_init(&pc->lru);

    vec_init_PageSlot(&pc->ptable);

    vec_init_slotid_t(&pc->free);
    for (slotid_t i = 0; i < CACHE_SLOTS; i++) {
        vec_push_slotid_t(&pc->free, i);
    }

    pc->pages = calloc(CACHE_SLOTS, sizeof(Page));

    return;
}

bool cache_new_page(PageCache *pc, Page **page) {
    pageid_t pid = disk_alloc(&pc->dm);

    if (!_try_get_page(pc, pid, page)) {
        disk_free(&pc->dm, pid);
        return false;
    }

    return true;
}

bool cache_fetch_page(PageCache *pc, pageid_t pid, Page **page) {
    slotid_t sid = 0;
    if (_find_cache_page(pc, pid, &sid)) {
        *page = &pc->pages[sid];
        (*page)->pins++;

        return true;
    }

    return _try_get_page(pc, pid, page);
}

void cache_unpin(PageCache *pc, Page *page) {
    if (--page->pins != 0) {
        return;
    }

    // Page is evictable
    slotid_t sid = 0;
    _find_cache_page(pc, page->pid, &sid);
    lru_set_evictable(&pc->lru, sid, true);

    return;
}

void cache_flush_page(PageCache *pc, Page *page) {
    disk_write(&pc->dm, page->pid, page->data);
    page->dirty = false;

    return;
}

void cache_close(PageCache *pc) {
    disk_close(&pc->dm);

    free(pc->lru.entries.data);
    free(pc->ptable.data);
    free(pc->free.data);
    free(pc->pages);

    *pc = (PageCache){0};

    return;
}

void lru_init(LRU *lru) {
    vec_init_LRUEntry(&lru->entries);

    return;
}

LRUEntry *lru_find_entry(const LRU *lru, slotid_t sid) {
    for (size_t i = 0; i < lru->entries.len; i++) {
        LRUEntry *entry = &lru->entries.data[i];
        if (entry->sid == sid) {
            return entry;
        }
    }

    return NULL;
}

void lru_register_entry(LRU *lru, slotid_t sid) {
    LRUEntry *entry = lru_find_entry(lru, sid);
    if (entry != NULL) {
        assert(entry->evictable);
        entry->evictable = false;
        entry->history = (LRUKHistory){0};

        return;
    }

    vec_push_LRUEntry(
        &lru->entries,
        (LRUEntry){.sid = sid, .evictable = false, .history = {0}});

    return;
}

void lru_access(LRU *lru, slotid_t sid) {
    LRUEntry *entry = lru_find_entry(lru, sid);
    assert(entry != NULL);

    LRUKHistory *history = &entry->history;
    if (history->len < LRUK + 1) {
        history->timestamps[history->len++] = lru->timestamp;
        return;
    }

    // Shift the array left one and assign the last element
    memcpy(&history->timestamps[0], &history->timestamps[1], LRUK);
    history->timestamps[LRUK] = lru->timestamp;
    lru->timestamp++;

    return;
}

bool lru_evict(LRU *lru, slotid_t *sid) {
    int max = 0;
    slotid_t max_sid = 0;

    // For entries where we can't look back K accesses:
    int oldest_lt_access = 0;
    slotid_t oldest_lt_sid = 0;

    for (size_t i = 0; i < lru->entries.len; i++) {
        LRUEntry entry = lru->entries.data[i];
        if (!entry.evictable) {
            continue;
        }

        if (entry.history.len < LRUK + 1) {
            int last_access = entry.history.timestamps[entry.history.len - 1];
            if (oldest_lt_access == 0 || last_access < oldest_lt_access) {
                oldest_lt_access = last_access;
                oldest_lt_sid = entry.sid;
            }

            continue;
        }

        int distance =
            entry.history.timestamps[LRUK] - entry.history.timestamps[0];
        if (distance > max) {
            max = distance;
            max_sid = entry.sid;
        }
    }

    if (max_sid != 0) {
        *sid = max_sid;
        return true;
    }

    if (oldest_lt_sid != 0) {
        *sid = oldest_lt_sid;
        return true;
    }

    return false;
}

void lru_set_evictable(LRU *lru, slotid_t sid, bool evictable) {
    LRUEntry *entry = lru_find_entry(lru, sid);
    assert(entry != NULL);
    entry->evictable = evictable;

    return;
}
