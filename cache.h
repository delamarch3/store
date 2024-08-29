#include <stdbool.h>
#include <stdlib.h>

#include "disk.h"
#include "vec.h"

#define CACHE_SLOTS 256
#define CACHE_SIZE PAGE_SIZE *CACHE_SLOTS

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
    int pins; // Avoid evicting pages that are pinned
    LRUKHistory history;
};
VEC_DEC(LRUEntry)

typedef struct LRU LRU;
struct LRU {
    // TODO: keep a reference count here?
    unsigned int timestamp;
    vec_LRUEntry entries;
};

typedef struct PageSlot PageSlot;
struct PageSlot {
    pageid_t pid;
    slotid_t sid;
};
VEC_DEC(PageSlot)

typedef struct PageCache PageCache;
struct PageCache {
    DiskManager dm;
    bool dirty[CACHE_SLOTS];
    vec_PageSlot ptable;
    LRU lru;
    vec_slotid_t free; // TODO: can be fixed size
    char *pages;
};

typedef struct Pin Pin;
struct Pin {
    pageid_t pid;
    slotid_t sid;
    LRU *lru;
    char *page;
};

void pagec_init(char *, PageCache *);
// Allocate a new page and attempt to find a slot in the cache. Returns false if
// there is no free or evictable page
bool pagec_new_page(PageCache *, Pin *);
// Fetch an allocated page and attempt to find a slot in the cache. Returns
// false if there is no free or evictable page
bool pagec_fetch_page(PageCache *, Pin *);
void pagec_flush_page(PageCache *, Pin *);
void pagec_free(PageCache *);

void lru_init(LRU *);
void lru_register_entry(LRU *, slotid_t);
void lru_access(LRU *, slotid_t);
bool lru_evict(LRU *, slotid_t *);
void lru_pin(LRU *, slotid_t);
void lru_unpin(LRU *, slotid_t);
