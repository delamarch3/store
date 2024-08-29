#define PAGE_SIZE 4096
typedef unsigned int pageid_t;

#define DISK_META_PAGE_ID 0
typedef struct DiskMeta DiskMeta;
struct DiskMeta {
    pageid_t next;
};

#define FREE_LIST_PAGE_ID 1
typedef struct FreeList FreeList;
struct FreeList {
    pageid_t next;
    unsigned int len;
    pageid_t pages[];
};

typedef struct DiskManager DiskManager;
struct DiskManager {
    int fd;
    DiskMeta *meta;
    FreeList *free;
};

void disk_open(char *, DiskManager *);
void disk_close(DiskManager *);
pageid_t disk_alloc(DiskManager *);
void disk_read(const DiskManager *, pageid_t, char *);
void disk_write(const DiskManager *, pageid_t, const char *);
void disk_free(DiskManager *, pageid_t);
