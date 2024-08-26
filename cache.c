#include <assert.h>
#include <string.h>

#include "cache.h"
#include "vec.h"

VEC_IMPL(slotid_t)
VEC_IMPL(LRUEntry)
VEC_IMPL(PageSlot)

static bool _find_cache_page(const struct PageCache *, pageid_t, slotid_t *);
static bool _find_cache_slot(const struct PageCache *, slotid_t, pageid_t *);
// Get safe offset within the cache
static char *_get_cache_offset(const struct PageCache *, const struct Pin *);
// Attempt to find a free/evictable slot in the page cache to hold the page
// specified in the pin. Returns false if there is no free or evictable page
static bool _try_get_page(struct PageCache *, struct Pin *);

static bool _find_cache_page(const struct PageCache *pc, pageid_t pid,
                             slotid_t *sid) {
    for (size_t i = 0; i < pc->ptable.len; i++) {
        if (pc->ptable.data[i].pid == pid) {
            *sid = pc->ptable.data[i].sid;
            return true;
        }
    }

    return false;
}

static bool _find_cache_slot(const struct PageCache *pc, slotid_t sid,
                             pageid_t *pid) {
    for (size_t i = 0; i < pc->ptable.len; i++) {
        if (pc->ptable.data[i].sid == sid) {
            *pid = pc->ptable.data[i].pid;
            return true;
        }
    }

    return false;
}

static char *_get_cache_offset(const struct PageCache *pc,
                               const struct Pin *pin) {
    pageid_t offset = pin->sid * PAGE_SIZE;
    assert(offset <= CACHE_SIZE - PAGE_SIZE);
    return pc->pages + offset;
}

static bool _try_get_page(struct PageCache *pc, struct Pin *pin) {
    // Try to find a free page
    slotid_t sid;
    if (!vec_pop_slotid_t(&pc->free, &sid) && !lru_evict(pc->lru, &sid)) {
        // There is no free or evicatable page
        return false;
    }

    // Register entry into LRU
    lru_register_entry(pc->lru, sid);
    lru_access(pc->lru, sid);

    // Write old page if dirty
    if (pc->dirty[sid]) {
        pageid_t pid;
        assert(_find_cache_slot(pc, sid, &pid));
        disk_write(pc->dm, pid, _get_cache_offset(pc, pin));
        pc->dirty[sid] = false;
    }

    // Read new page
    disk_read(pc->dm, pin->pid, _get_cache_offset(pc, pin));

    // Insert pid -> sid into page table
    vec_push_PageSlot(&pc->ptable, (PageSlot){.pid = pin->pid, .sid = sid});

    return true;
}

void pagec_init(char *path, struct PageCache *pc) {
    struct DiskManager dm;
    disk_open(path, &dm);

    pc->pages = (char *)malloc(CACHE_SIZE);

    vec_init_slotid_t(&pc->free);
    for (slotid_t i = 0; i < CACHE_SLOTS; i++) {
        vec_push_slotid_t(&pc->free, i);
    }

    return;
}

bool pagec_new_page(struct PageCache *pc, struct Pin *pin) {
    pin->pid = disk_alloc(pc->dm);

    if (!_try_get_page(pc, pin)) {
        disk_free(pc->dm, pin->pid);
        pin->pid = 0;
        return false;
    }

    return true;
}

bool pagec_fetch_page(struct PageCache *pc, struct Pin *pin) {
    if (!_find_cache_page(pc, pin->pid, &pin->sid) && !_try_get_page(pc, pin)) {
        return false;
    }

    pin->lru = pc->lru;
    pin->page = _get_cache_offset(pc, pin);

    return true;
}

void lru_register_entry(LRU *lru, slotid_t sid) {
    for (size_t i = 0; i < lru->entries.len; i++) {
        LRUEntry *entry = &lru->entries.data[i];
        if (entry->sid == sid) {
            // Reuse this entry
            assert(entry->pins == 0);
            entry->history = (LRUKHistory){0};

            return;
        }
    }

    vec_push_LRUEntry(&lru->entries, (LRUEntry){.sid = sid, .history = {0}});
}

void lru_access(LRU *lru, slotid_t sid) {
    bool found = false;
    for (size_t i = 0; i < lru->entries.len; i++) {
        LRUEntry *entry = &lru->entries.data[i];
        if (entry->sid == sid) {
            found = true;

            LRUKHistory *history = &entry->history;
            if (history->len < LRUK + 1) {
                history->timestamps[history->len++] = lru->timestamp;
                break;
            }

            // Shift the array left one and assign the last element
            memcpy(&history->timestamps[0], &history->timestamps[1], LRUK);
            history->timestamps[LRUK] = lru->timestamp;

            lru->timestamp++;
        }
    }

    assert(found);
}

bool lru_evict(const LRU *lru, slotid_t *sid) {
    int max = 0;
    slotid_t max_sid = 0;

    // For entries where we can't look back K accesses:
    int oldest_lt_access = 0;
    slotid_t oldest_lt_sid = 0;

    for (size_t i = 0; i < lru->entries.len; i++) {
        LRUEntry entry = lru->entries.data[i];
        if (entry.pins != 0) {
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
