#define PAGE_SIZE 4096
typedef unsigned int pageid_t;

#define DISK_META_PAGE_ID 0
struct DiskMeta {
    pageid_t tail;
};

#define FREE_LIST_PAGE_ID 1
struct FreeList {
    pageid_t next;
    unsigned int len;
    pageid_t pages[];
};

struct DiskManager {
    int fd;
    struct DiskMeta *meta;
    struct FreeList *free;
};

void disk_open(char *, struct DiskManager *);
void disk_close(struct DiskManager *);
pageid_t disk_alloc(struct DiskManager *);
void disk_read(const struct DiskManager *, pageid_t, char *);
void disk_write(const struct DiskManager *, pageid_t, const char *);
void disk_free_page(struct DiskManager *, pageid_t);
