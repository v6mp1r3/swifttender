/*
 * file_io.c -- persistence layer stub
 * Full implementation in: feat: define all C structs and file I/O layer
 */
#include "file_io.h"
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>

int file_io_init(const char *data_dir) {
    /* Create data directory if it does not exist */
    struct stat st;
    if (stat(data_dir, &st) != 0) {
        if (mkdir(data_dir, 0755) != 0 && errno != EEXIST) {
            perror("[file_io] mkdir");
            return -1;
        }
    }
    printf("[file_io] Data directory ready: %s\n", data_dir);
    return 0;
}
