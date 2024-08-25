#include <stdbool.h>
#include <stdlib.h>

#include "disk.h"
#include "vec.h"

#define CACHE_SLOTS 256
#define CACHE_SIZE PAGE_SIZE *CACHE_SLOTS

typedef size_t slotid_t;
VEC(slotid_t)

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
VEC(LRUEntry)

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
VEC(PageSlot)

#define PTABLE_SIZE 8
struct PageCache {
    struct DiskManager *dm;
    slotid_t dirty[CACHE_SLOTS];
    vec_PageSlot ptable;
    LRU *lru;
    vec_slotid_t free;
    char *pages;
};

struct Pin {
    pageid_t pid;
    slotid_t sid;
    LRU *lru;
    char *page;
} Pin;

void pagec_init(char *, struct PageCache *);
// Allocate a new page and attempt to find a slot in the cache. If there is
// no free or evictable page, pid will remain unset
void pagec_new_page(struct PageCache *pc, struct Pin *pin);
// Fetch an allocated page and attempt to find a slot in the cache. If there
// is no free or evictable page, sid will remain unset
void pagec_fetch_page(struct PageCache *pc, struct Pin *pin);

void lru_register_entry(LRU *, slotid_t);
void lru_access(LRU *, slotid_t);
bool lru_evict(const LRU *, slotid_t *);
