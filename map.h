#include "cache.h"
#include "disk.h"

typedef struct Map Map;
struct Map {
    PageCache *pc;
    pageid_t directory_pid;
};

// Pointers to buckets
typedef struct Directory Directory;
struct Directory {
    size_t global_depth; /* used to compute the index of a hash */
    pageid_t buckets[];
};

// Holds the key/value pairs
typedef struct Bucket Bucket;
struct Bucket {
    size_t local_depth;
    size_t len;  /* len of key/value pairs */
    size_t size; /* size of key/value pairs */
    char data[]; /* data is encoded keylen|vallen|key|val ... */
};

typedef struct BucketIter BucketIter;
struct BucketIter {
    char *current;   /* ptr inside bucket data */
    size_t rem_len;  /* remaining len of entries */
    size_t rem_size; /* remaining size of entries */
};

bool bucket_put(Bucket *, char *, size_t, char *, size_t);

void bucket_iter_init(Bucket *, BucketIter *);
bool bucket_iter_next(BucketIter *, char **, size_t *, char **, size_t *);

void map_init(Map *, PageCache *);
bool map_insert(Map *, char *, size_t, char *, size_t);
bool map_get(Map *, char *, size_t, char **, size_t *);
