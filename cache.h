#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "disk.h"
#include "vec.h"

#define CACHE_SLOTS 256

typedef size_t slotid_t;
VEC_DEC(slotid_t)

#define LRUK 2
typedef struct LRUKHistory LRUKHistory;
struct LRUKHistory {
    int len;
    unsigned int timestamps[LRUK + 1];
};

typedef struct LRUEntry LRUEntry;
struct LRUEntry {
    slotid_t sid;
    bool evictable;
    LRUKHistory history;
};
VEC_DEC(LRUEntry)

typedef struct LRU LRU;
struct LRU {
    unsigned int timestamp;
    vec_LRUEntry entries;
};

typedef struct PageSlot PageSlot;
struct PageSlot {
    pageid_t pid;
    slotid_t sid;
};
VEC_DEC(PageSlot)

typedef struct Page Page;
struct Page {
    pageid_t pid;
    // TODO: lock
    int pins; // reference count
    bool dirty;
    int __padding;

    char data[PAGE_SIZE];
};

typedef struct PageCache PageCache;
struct PageCache {
    DiskManager dm;
    vec_PageSlot ptable;
    LRU lru;
    vec_slotid_t free; // TODO: can be fixed size
    Page *pages;
};

void cache_init(char *, PageCache *);
// Allocate a new page and attempt to find a slot in the cache. Returns false if
// there is no free or evictable page
bool cache_new_page(PageCache *, Page **);
// Fetch an allocated page and attempt to find a slot in the cache. Returns
// false if there is no free or evictable page
bool cache_fetch_page(PageCache *, pageid_t, Page **);
bool cache_fetch_or_set(PageCache *, pageid_t *, Page **);
void cache_unpin(PageCache *, Page *);
void cache_flush_page(PageCache *, Page *);
void cache_close(PageCache *);

void lru_init(LRU *);
LRUEntry *lru_find_entry(const LRU *, slotid_t);
void lru_register_entry(LRU *, slotid_t);
void lru_access(LRU *, slotid_t);
bool lru_evict(LRU *, slotid_t *);
void lru_set_evictable(LRU *, slotid_t, bool);
