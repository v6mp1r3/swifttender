/*
 * SwiftTender -- file_io.c  (fixed: heap allocation for large buffers)
 *
 * The original used stack-allocated arrays like Tender buf[4096] which
 * caused a stack overflow on macOS (4096 * ~1700 bytes ≈ 7 MB > 8 MB limit).
 * All large temporary buffers now use malloc/free.
 */

#include "file_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

static char s_data_dir[512] = "./data";

static void path_for(const char *filename, char *out, size_t max) {
    snprintf(out, max, "%s/%s", s_data_dir, filename);
}

static void ensure_file(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) return;
    FILE *f = fopen(path, "wb");
    if (f) fclose(f);
    else fprintf(stderr, "[file_io] Could not create %s: %s\n",
                 path, strerror(errno));
}

/* ── Init ────────────────────────────────────────────────────────── */
int file_io_init(const char *data_dir) {
    if (!data_dir) return -1;
    strncpy(s_data_dir, data_dir, sizeof(s_data_dir) - 1);

    struct stat st;
    if (stat(s_data_dir, &st) != 0) {
        if (mkdir(s_data_dir, 0755) != 0 && errno != EEXIST) {
            perror("[file_io] mkdir"); return -1;
        }
    }

    char uploads[600];
    snprintf(uploads, sizeof(uploads), "%s/uploads", s_data_dir);
    mkdir(uploads, 0755);

    char p[600];
    path_for(FIO_USERS_FILE,     p, sizeof(p)); ensure_file(p);
    path_for(FIO_TENDERS_FILE,   p, sizeof(p)); ensure_file(p);
    path_for(FIO_OFFERS_FILE,    p, sizeof(p)); ensure_file(p);
    path_for(FIO_CONTRACTS_FILE, p, sizeof(p)); ensure_file(p);

    printf("[file_io] Initialised data directory: %s\n", s_data_dir);
    return 0;
}

/* ── Generic helpers (heap-allocated) ───────────────────────────── */
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
 * generic_load_heap -- reads up to `max` records into a HEAP-ALLOCATED
 * buffer pointed to by *buf_out. Caller must free(*buf_out).
 * Returns number of records read, or 0 on failure.
 *
 * FIX: replaces stack-allocated arrays that caused macOS stack overflow.
 */
static size_t generic_load_heap(const char *filepath, size_t record_size,
                                 size_t max, void **buf_out) {
    *buf_out = NULL;
    if (max == 0) return 0;

    void *buf = malloc(record_size * max);
    if (!buf) {
        fprintf(stderr, "[file_io] malloc failed for %s\n", filepath);
        return 0;
    }

    FILE *f = fopen(filepath, "rb");
    if (!f) { free(buf); *buf_out = buf; return 0; }

    size_t count = fread(buf, record_size, max, f);
    fclose(f);
    *buf_out = buf;
    return count;
}

static int generic_update(const char *filepath, const void *record,
                           size_t size, size_t index) {
    FILE *f = fopen(filepath, "r+b");
    if (!f) return -1;
    if (fseek(f, (long)(index * size), SEEK_SET) != 0) {
        fclose(f); return -1;
    }
    size_t written = fwrite(record, size, 1, f);
    fclose(f);
    return (written == 1) ? 0 : -1;
}

/* ── Users ───────────────────────────────────────────────────────── */
int fio_user_append(const User *u) {
    char p[600]; path_for(FIO_USERS_FILE, p, sizeof(p));
    return generic_append(p, u, sizeof(User));
}

size_t fio_user_load_all(User *out, size_t max) {
    char p[600]; path_for(FIO_USERS_FILE, p, sizeof(p));
    void *buf = NULL;
    size_t n = generic_load_heap(p, sizeof(User), max, &buf);
    if (buf && n > 0) memcpy(out, buf, n * sizeof(User));
    free(buf);
    return n;
}

int fio_user_find_id(uint32_t id, User *out) {
    User *buf = malloc(sizeof(User) * FIO_MAX_USERS);
    if (!buf) return -1;
    size_t n = fio_user_load_all(buf, FIO_MAX_USERS);
    int found = -1;
    for (size_t i = 0; i < n; i++) {
        if (buf[i].active && buf[i].id == id) {
            *out = buf[i]; found = 0; break;
        }
    }
    free(buf);
    return found;
}

int fio_user_find_email(const char *email, User *out) {
    User *buf = malloc(sizeof(User) * FIO_MAX_USERS);
    if (!buf) return -1;
    size_t n = fio_user_load_all(buf, FIO_MAX_USERS);
    int found = -1;
    for (size_t i = 0; i < n; i++) {
        if (buf[i].active &&
            strncmp(buf[i].email, email, sizeof(buf[i].email)) == 0) {
            *out = buf[i]; found = 0; break;
        }
    }
    free(buf);
    return found;
}

int fio_user_find_idno(const char *idno, User *out) {
    User *buf = malloc(sizeof(User) * FIO_MAX_USERS);
    if (!buf) return -1;
    size_t n = fio_user_load_all(buf, FIO_MAX_USERS);
    int found = -1;
    for (size_t i = 0; i < n; i++) {
        if (buf[i].active &&
            strncmp(buf[i].idno, idno, sizeof(buf[i].idno)) == 0) {
            *out = buf[i]; found = 0; break;
        }
    }
    free(buf);
    return found;
}

int fio_user_update(const User *u) {
    char p[600]; path_for(FIO_USERS_FILE, p, sizeof(p));
    User *buf = malloc(sizeof(User) * FIO_MAX_USERS);
    if (!buf) return -1;
    void *raw = NULL;
    size_t n = generic_load_heap(p, sizeof(User), FIO_MAX_USERS, &raw);
    if (raw) memcpy(buf, raw, n * sizeof(User));
    free(raw);
    int result = -1;
    for (size_t i = 0; i < n; i++) {
        if (buf[i].id == u->id) {
            result = generic_update(p, u, sizeof(User), i);
            break;
        }
    }
    free(buf);
    return result;
}

/* ── Tenders ─────────────────────────────────────────────────────── */
int fio_tender_append(const Tender *t) {
    char p[600]; path_for(FIO_TENDERS_FILE, p, sizeof(p));
    return generic_append(p, t, sizeof(Tender));
}

size_t fio_tender_load_all(Tender *out, size_t max) {
    char p[600]; path_for(FIO_TENDERS_FILE, p, sizeof(p));
    void *raw = NULL;
    size_t total = generic_load_heap(p, sizeof(Tender), FIO_MAX_TENDERS, &raw);
    Tender *buf  = (Tender *)raw;
    size_t count = 0;
    for (size_t i = 0; i < total && count < max; i++) {
        if (buf[i].active) out[count++] = buf[i];
    }
    free(raw);
    return count;
}

int fio_tender_find_id(uint32_t id, Tender *out) {
    Tender *buf = malloc(sizeof(Tender) * FIO_MAX_TENDERS);
    if (!buf) return -1;
    size_t n = fio_tender_load_all(buf, FIO_MAX_TENDERS);
    int found = -1;
    for (size_t i = 0; i < n; i++) {
        if (buf[i].id == id) { *out = buf[i]; found = 0; break; }
    }
    free(buf);
    return found;
}

int fio_tender_update(const Tender *t) {
    char p[600]; path_for(FIO_TENDERS_FILE, p, sizeof(p));
    void *raw = NULL;
    size_t n = generic_load_heap(p, sizeof(Tender), FIO_MAX_TENDERS, &raw);
    Tender *buf = (Tender *)raw;
    int result = -1;
    for (size_t i = 0; i < n; i++) {
        if (buf[i].id == t->id) {
            result = generic_update(p, t, sizeof(Tender), i);
            break;
        }
    }
    free(raw);
    return result;
}

/* ── Offers ──────────────────────────────────────────────────────── */
int fio_offer_append(const Offer *o) {
    char p[600]; path_for(FIO_OFFERS_FILE, p, sizeof(p));
    return generic_append(p, o, sizeof(Offer));
}

int fio_offer_find_id(uint32_t id, Offer *out) {
    char p[600]; path_for(FIO_OFFERS_FILE, p, sizeof(p));
    void *raw = NULL;
    size_t n = generic_load_heap(p, sizeof(Offer), FIO_MAX_OFFERS, &raw);
    Offer *buf = (Offer *)raw;
    int found = -1;
    for (size_t i = 0; i < n; i++) {
        if (buf[i].active && buf[i].id == id) {
            *out = buf[i]; found = 0; break;
        }
    }
    free(raw);
    return found;
}

int fio_offer_update(const Offer *o) {
    char p[600]; path_for(FIO_OFFERS_FILE, p, sizeof(p));
    void *raw = NULL;
    size_t n = generic_load_heap(p, sizeof(Offer), FIO_MAX_OFFERS, &raw);
    Offer *buf = (Offer *)raw;
    int result = -1;
    for (size_t i = 0; i < n; i++) {
        if (buf[i].id == o->id) {
            result = generic_update(p, o, sizeof(Offer), i);
            break;
        }
    }
    free(raw);
    return result;
}

size_t fio_offer_load_by_tender(uint32_t tender_id,
                                 Offer *out, size_t max) {
    char p[600]; path_for(FIO_OFFERS_FILE, p, sizeof(p));
    void *raw = NULL;
    size_t total = generic_load_heap(p, sizeof(Offer), FIO_MAX_OFFERS, &raw);
    Offer *buf = (Offer *)raw;
    size_t count = 0;
    for (size_t i = 0; i < total && count < max; i++) {
        if (buf[i].active && buf[i].tender_id == tender_id)
            out[count++] = buf[i];
    }
    free(raw);
    return count;
}

/* ── Audit log ───────────────────────────────────────────────────── */
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

/* ── ID generation ───────────────────────────────────────────────── */
uint32_t fio_next_id(void) {
    static uint32_t counter = 0;
    return (uint32_t)time(NULL) + (++counter);
}
