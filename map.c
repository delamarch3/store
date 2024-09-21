#include "map.h"

static size_t hash(char *);

bool bucket_put(Bucket *bucket, char *key, size_t klen, char *value,
                size_t vlen) {
    bucket->size += sizeof(size_t) + sizeof(size_t) + klen + vlen;
    assert(bucket->size <= PAGE_SIZE);
    bucket->len++;

    memcpy(bucket->data + bucket->size, &klen, sizeof(size_t));
    memcpy(bucket->data + bucket->size + sizeof(size_t), &vlen, sizeof(size_t));
    memcpy(bucket->data + bucket->size + sizeof(size_t) + sizeof(size_t), key,
           klen);
    memcpy(bucket->data + bucket->size + sizeof(size_t) + sizeof(size_t) + klen,
           value, vlen);

    return true;
}

void bucket_iter_init(Bucket *bucket, BucketIter *iter) {
    iter->current = bucket->data;
    iter->rem_len = bucket->len;
    iter->rem_size = bucket->size;
}

bool bucket_iter_next(BucketIter *iter, char **key, size_t *klen, char **value,
                      size_t *vlen) {
    if (iter->rem_len == 0) {
        assert(iter->rem_size == 0);
        return false;
    }

    iter->rem_len--;

    size_t size = 0;
    memcpy(klen, iter->current + size, sizeof(size_t));
    size += sizeof(size_t);
    memcpy(vlen, iter->current + size, sizeof(size_t));
    size += sizeof(size_t);

    *key = iter->current + size;
    size += *klen;
    *value = iter->current + size;
    size += *vlen;

    iter->current += size;
    iter->rem_size -= size;

    return true;
}

static size_t hash(char *str) {
    size_t hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

void map_init(Map *map, PageCache *pc) {
    map->pc = pc;
    map->directory_pid = 0;
}

bool map_insert(Map *map, char *key, size_t klen, char *value, size_t vlen) {
    Page *directory_page = NULL;
    if (!cache_fetch_or_set(map->pc, &map->directory_pid, &directory_page)) {
        return false;
    };
    Directory *directory = (Directory *)directory_page->data;

    size_t i = hash(key) & ((1 << directory->global_depth) - 1);
    Page *bucket_page = NULL;
    if (!cache_fetch_or_set(map->pc, &directory->buckets[i], &bucket_page)) {
        return false;
    }
    Bucket *bucket = (Bucket *)bucket_page->data;

    const size_t entry_size = klen + vlen;
    bool full = sizeof(Bucket) + bucket->size + entry_size >= PAGE_SIZE;
    if (!full) {
        bucket_put(bucket, key, klen, value, vlen);
        return true;
    }

    /* split the bucket */

    if (bucket->local_depth == directory->global_depth) {
        directory->global_depth++;
    }

    Page *page0, *page1 = NULL;
    if (!cache_new_page(map->pc, &page0)) {
        return false;
    };
    if (!cache_new_page(map->pc, &page1)) {
        // TODO: free p0
        return false;
    };

    Bucket *bucket0 = (Bucket *)page0->data;
    Bucket *bucket1 = (Bucket *)page1->data;
    bucket0->local_depth = bucket1->local_depth = bucket->local_depth + 1;

    // split entries between the new buckets
    int high_bit = 1 << bucket->local_depth;
    BucketIter iter = {0};
    bucket_iter_init(bucket, &iter);
    char **ikey, **ivalue = NULL;
    size_t iklen, ivlen = 0;
    while (bucket_iter_next(&iter, ikey, &iklen, ivalue, &ivlen)) {
        if (hash(*ikey) & high_bit) {
            bucket_put(bucket1, *ikey, iklen, *ivalue, ivlen);
        } else {
            bucket_put(bucket0, *ikey, iklen, *ivalue, ivlen);
        }
    }

    // insert requested entry
    if (hash(key) & high_bit) {
        bucket_put(bucket1, key, klen, value, vlen);
    } else {
        bucket_put(bucket0, key, klen, value, vlen);
    }

    // insert buckets into directory
    for (size_t i = hash(key) & (high_bit - 1);
         i < (1 < directory->global_depth); i += high_bit) {
        if (i & high_bit) {
            directory->buckets[i] = page1->pid;
        } else {
            directory->buckets[i] = page0->pid;
        }
    }

    // TODO: free bucket page for reuse

    return true;
}
