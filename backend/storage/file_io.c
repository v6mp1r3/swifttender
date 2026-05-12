/*
 * ==================================================================
 *  SwiftTender -- File I/O Persistence Layer Implementation
 *  storage/file_io.c
 * ==================================================================
 */

#include "file_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

/* ----------------------------------------------------------------
 * Module-level state: data directory path set by file_io_init().
 * All path-building functions use this as a prefix.
 * ---------------------------------------------------------------- */
static char s_data_dir[512] = "./data";

/* ----------------------------------------------------------------
 * path_for -- build a full path: s_data_dir + "/" + filename.
 * Writes into caller-supplied buffer `out` of size `max`.
 * ---------------------------------------------------------------- */
static void path_for(const char *filename, char *out, size_t max) {
    snprintf(out, max, "%s/%s", s_data_dir, filename);
}

/* ----------------------------------------------------------------
 * ensure_file -- create an empty file at `path` if it does not exist.
 * Used by file_io_init() to create .bin files on first run.
 * ---------------------------------------------------------------- */
static void ensure_file(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) return;   /* already exists */
    FILE *f = fopen(path, "wb");
    if (f) fclose(f);
    else fprintf(stderr, "[file_io] Could not create %s: %s\n",
                 path, strerror(errno));
}

/* ----------------------------------------------------------------
 * file_io_init -- create data directory and initialise empty files.
 * ---------------------------------------------------------------- */
int file_io_init(const char *data_dir) {
    if (!data_dir) return -1;
    strncpy(s_data_dir, data_dir, sizeof(s_data_dir) - 1);

    /* Create directory if missing */
    struct stat st;
    if (stat(s_data_dir, &st) != 0) {
        if (mkdir(s_data_dir, 0755) != 0 && errno != EEXIST) {
            perror("[file_io] mkdir");
            return -1;
        }
    }

    /* Create uploads subdirectory */
    char uploads[600];
    snprintf(uploads, sizeof(uploads), "%s/uploads", s_data_dir);
    mkdir(uploads, 0755);

    /* Ensure all binary files exist */
    char p[600];
    path_for(FIO_USERS_FILE,     p, sizeof(p)); ensure_file(p);
    path_for(FIO_TENDERS_FILE,   p, sizeof(p)); ensure_file(p);
    path_for(FIO_OFFERS_FILE,    p, sizeof(p)); ensure_file(p);
    path_for(FIO_CONTRACTS_FILE, p, sizeof(p)); ensure_file(p);

    printf("[file_io] Initialised data directory: %s\n", s_data_dir);
    return 0;
}

/* ================================================================
 * GENERIC HELPERS
 *
 * Since all entity files use the same pattern (flat binary array of
 * fixed-size structs), we write two internal helpers:
 *
 *   generic_append  -- append one struct to the end of a file
 *   generic_load    -- read all structs from a file into an array
 *
 * Entity-specific functions call these with the right file name and
 * struct size.
 * ================================================================ */

/*
 * generic_append -- append `size` bytes from `record` to `filepath`.
 *
 * Opens in "ab" (append binary) mode so the file pointer starts at
 * the end. fwrite() writes exactly one record.
 *
 * Returns 0 on success, -1 on failure.
 */
static int generic_append(const char *filepath,
                           const void *record, size_t size) {
    FILE *f = fopen(filepath, "ab");
    if (!f) {
        fprintf(stderr, "[file_io] append open failed: %s\n", filepath);
        return -1;
    }
    size_t written = fwrite(record, size, 1, f);
    fclose(f);
    return (written == 1) ? 0 : -1;
}

/*
 * generic_load -- read all records from `filepath` into `out`.
 *
 * Reads up to `max` records of `size` bytes each.
 * Returns the number of records actually read.
 */
static size_t generic_load(const char *filepath,
                            void *out, size_t size, size_t max) {
    FILE *f = fopen(filepath, "rb");
    if (!f) return 0;
    size_t count = fread(out, size, max, f);
    fclose(f);
    return count;
}

/*
 * generic_update -- overwrite a specific record in the file by index.
 *
 * Finds the record at byte offset (index * size), seeks there, and
 * overwrites it. This is O(1) random access — a key advantage of
 * fixed-size struct files over variable-length formats.
 *
 * Returns 0 on success, -1 if index is out of range or I/O fails.
 */
static int generic_update(const char *filepath,
                           const void *record, size_t size,
                           size_t index) {
    FILE *f = fopen(filepath, "r+b");
    if (!f) return -1;

    if (fseek(f, (long)(index * size), SEEK_SET) != 0) {
        fclose(f); return -1;
    }
    size_t written = fwrite(record, size, 1, f);
    fclose(f);
    return (written == 1) ? 0 : -1;
}

/* ================================================================
 * USER OPERATIONS
 * ================================================================ */

int fio_user_append(const User *u) {
    char p[600]; path_for(FIO_USERS_FILE, p, sizeof(p));
    return generic_append(p, u, sizeof(User));
}

size_t fio_user_load_all(User *out, size_t max) {
    char p[600]; path_for(FIO_USERS_FILE, p, sizeof(p));
    return generic_load(p, out, sizeof(User), max);
}

int fio_user_find_id(uint32_t id, User *out) {
    User buf[FIO_MAX_USERS];
    size_t n = fio_user_load_all(buf, FIO_MAX_USERS);
    for (size_t i = 0; i < n; i++) {
        if (buf[i].active && buf[i].id == id) {
            *out = buf[i]; return 0;
        }
    }
    return -1;
}

int fio_user_find_email(const char *email, User *out) {
    User buf[FIO_MAX_USERS];
    size_t n = fio_user_load_all(buf, FIO_MAX_USERS);
    for (size_t i = 0; i < n; i++) {
        if (buf[i].active &&
            strncmp(buf[i].email, email, sizeof(buf[i].email)) == 0) {
            *out = buf[i]; return 0;
        }
    }
    return -1;
}

int fio_user_find_idno(const char *idno, User *out) {
    User buf[FIO_MAX_USERS];
    size_t n = fio_user_load_all(buf, FIO_MAX_USERS);
    for (size_t i = 0; i < n; i++) {
        if (buf[i].active &&
            strncmp(buf[i].idno, idno, sizeof(buf[i].idno)) == 0) {
            *out = buf[i]; return 0;
        }
    }
    return -1;
}

int fio_user_update(const User *u) {
    User buf[FIO_MAX_USERS];
    char p[600]; path_for(FIO_USERS_FILE, p, sizeof(p));
    size_t n = generic_load(p, buf, sizeof(User), FIO_MAX_USERS);
    for (size_t i = 0; i < n; i++) {
        if (buf[i].id == u->id) {
            return generic_update(p, u, sizeof(User), i);
        }
    }
    return -1;
}

/* ================================================================
 * TENDER OPERATIONS
 * ================================================================ */

int fio_tender_append(const Tender *t) {
    char p[600]; path_for(FIO_TENDERS_FILE, p, sizeof(p));
    return generic_append(p, t, sizeof(Tender));
}

size_t fio_tender_load_all(Tender *out, size_t max) {
    char p[600]; path_for(FIO_TENDERS_FILE, p, sizeof(p));
    Tender buf[FIO_MAX_TENDERS];
    size_t total = generic_load(p, buf, sizeof(Tender), FIO_MAX_TENDERS);
    size_t count = 0;
    /* Filter: only return active (non-soft-deleted) tenders */
    for (size_t i = 0; i < total && count < max; i++) {
        if (buf[i].active) out[count++] = buf[i];
    }
    return count;
}

int fio_tender_find_id(uint32_t id, Tender *out) {
    Tender buf[FIO_MAX_TENDERS];
    size_t n = fio_tender_load_all(buf, FIO_MAX_TENDERS);
    for (size_t i = 0; i < n; i++) {
        if (buf[i].id == id) { *out = buf[i]; return 0; }
    }
    return -1;
}

int fio_tender_update(const Tender *t) {
    Tender buf[FIO_MAX_TENDERS];
    char p[600]; path_for(FIO_TENDERS_FILE, p, sizeof(p));
    size_t n = generic_load(p, buf, sizeof(Tender), FIO_MAX_TENDERS);
    for (size_t i = 0; i < n; i++) {
        if (buf[i].id == t->id) {
            return generic_update(p, t, sizeof(Tender), i);
        }
    }
    return -1;
}

/* ================================================================
 * OFFER OPERATIONS
 * ================================================================ */

int fio_offer_append(const Offer *o) {
    char p[600]; path_for(FIO_OFFERS_FILE, p, sizeof(p));
    return generic_append(p, o, sizeof(Offer));
}

int fio_offer_find_id(uint32_t id, Offer *out) {
    Offer buf[FIO_MAX_OFFERS];
    char p[600]; path_for(FIO_OFFERS_FILE, p, sizeof(p));
    size_t n = generic_load(p, buf, sizeof(Offer), FIO_MAX_OFFERS);
    for (size_t i = 0; i < n; i++) {
        if (buf[i].active && buf[i].id == id) {
            *out = buf[i]; return 0;
        }
    }
    return -1;
}

int fio_offer_update(const Offer *o) {
    Offer buf[FIO_MAX_OFFERS];
    char p[600]; path_for(FIO_OFFERS_FILE, p, sizeof(p));
    size_t n = generic_load(p, buf, sizeof(Offer), FIO_MAX_OFFERS);
    for (size_t i = 0; i < n; i++) {
        if (buf[i].id == o->id) {
            return generic_update(p, o, sizeof(Offer), i);
        }
    }
    return -1;
}

size_t fio_offer_load_by_tender(uint32_t tender_id,
                                 Offer *out, size_t max) {
    Offer buf[FIO_MAX_OFFERS];
    char p[600]; path_for(FIO_OFFERS_FILE, p, sizeof(p));
    size_t total = generic_load(p, buf, sizeof(Offer), FIO_MAX_OFFERS);
    size_t count = 0;
    for (size_t i = 0; i < total && count < max; i++) {
        if (buf[i].active && buf[i].tender_id == tender_id)
            out[count++] = buf[i];
    }
    return count;
}

/* ================================================================
 * AUDIT LOG
 * ================================================================ */

/*
 * fio_audit_append -- append one line to audit.log.
 *
 * Format: [ISO timestamp] user_id=N action entity_id=N detail
 * Opened in "a" (append text) mode so writes are always at the end.
 * The OS guarantees append mode writes are atomic for small buffers.
 */
void fio_audit_append(uint32_t user_id, const char *action,
                       uint32_t entity_id, const char *detail) {
    char p[600]; path_for(FIO_AUDIT_FILE, p, sizeof(p));

    FILE *f = fopen(p, "a");
    if (!f) return;

    time_t now = time(NULL);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));

    fprintf(f, "[%s] user=%u action=%s entity=%u %s\n",
            ts, user_id, action ? action : "-",
            entity_id, detail ? detail : "");
    fclose(f);
}

/* ================================================================
 * ID GENERATION
 * ================================================================ */

/*
 * fio_next_id -- generate a unique uint32_t ID.
 *
 * Uses epoch seconds as a base and a static counter to handle
 * multiple IDs generated in the same second. Not cryptographically
 * random, but unique and monotonically increasing for a single-
 * server prototype.
 */
uint32_t fio_next_id(void) {
    static uint32_t counter = 0;
    return (uint32_t)time(NULL) + (++counter);
}
