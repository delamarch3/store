#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "disk.h"

static bool _page_in_free_list(const FreeList *free_list, pageid_t pid) {
    for (unsigned int i = 0; i < free_list->len; i++) {
        if (free_list->pages[i] == pid) {
            return true;
        }
    }

    return false;
}

static void _disk_read(const DiskManager *dm, pageid_t pid, char *data) {
    ssize_t nbyte = pread(dm->fd, data, PAGE_SIZE, pid * PAGE_SIZE);
    if (nbyte == -1) {
        printf("could not seek read page: %s\n", strerror(errno));
        exit(1);
    } else if (nbyte != PAGE_SIZE && nbyte != 0) {
        printf("did not read full page, read: %zd\n", nbyte);
        exit(1);
    }

    return;
}

static void _disk_write(const DiskManager *dm, pageid_t pid, const char *data) {
    ssize_t nbyte = pwrite(dm->fd, data, PAGE_SIZE, pid * PAGE_SIZE);
    if (nbyte == -1) {
        printf("could not write page: %s\n", strerror(errno));
        exit(1);
    } else if (nbyte != PAGE_SIZE) {
        printf("did not write full page, written: %zd\n", nbyte);
        exit(1);
    }

    return;
}

void disk_open(char *path, DiskManager *dm) {
    dm->fd = open(path, O_RDWR | O_CREAT);
    if (dm->fd == -1) {
        printf("could not open %s: %s", path, strerror(errno));
        exit(1);
    }

    dm->meta = calloc(1, PAGE_SIZE);
    _disk_read(dm, DISK_META_PAGE_ID, (char *)dm->meta);
    if (dm->meta->next == 0) {
        dm->meta->next = FREE_LIST_PAGE_ID;
    }

    dm->free = calloc(1, PAGE_SIZE);
    _disk_read(dm, FREE_LIST_PAGE_ID, (char *)dm->free);

    return;
}

void disk_close(DiskManager *dm) {
    _disk_write(dm, DISK_META_PAGE_ID, (char *)dm->meta);
    _disk_write(dm, FREE_LIST_PAGE_ID, (char *)dm->free);

    free(dm->meta);
    free(dm->free);

    if (close(dm->fd) == -1) {
        printf("could not close disk: %s", strerror(errno));
        exit(1);
    }

    *dm = (DiskManager){0};

    return;
}

pageid_t disk_alloc(DiskManager *dm) {
    if (dm->free->len > 0) {
        printf("%d\n", dm->free->len);
        return dm->free->pages[--dm->free->len];
    }

    return ++dm->meta->next;
}

void disk_read(const DiskManager *dm, pageid_t pid, char *data) {
    if (_page_in_free_list(dm->free, pid)) {
        printf("attempt to read freed page %d\n", pid);
        exit(1);
    }

    _disk_read(dm, pid, data);

    return;
}

void disk_write(const DiskManager *dm, pageid_t pid, const char *data) {
    if (_page_in_free_list(dm->free, pid)) {
        printf("attempt to write freed page %d\n", pid);
        exit(1);
    }

    _disk_write(dm, pid, data);

    return;
}

void disk_free(DiskManager *dm, pageid_t pid) {
    dm->free->pages[dm->free->len++] = pid;

    // TODO: clear the page?
    // TODO: check if a new page needs to be allocated to hold the freed pid

    return;
}
