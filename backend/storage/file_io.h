#ifndef FILE_IO_H
#define FILE_IO_H

#include <stddef.h>
#include "../models/user.h"
#include "../models/tender.h"
#include "../models/offer.h"
#include "../models/notification.h"

/*
 * ==================================================================
 *  SwiftTender -- File I/O Persistence Layer
 *  storage/file_io.h
 * ==================================================================
 *
 * PURPOSE
 * -------
 * All disk access is isolated in this module. No other module touches
 * files directly. This makes the persistence strategy easy to swap
 * (e.g. SQLite) without changing business logic.
 *
 * STORAGE FORMAT
 * --------------
 * Each entity type is stored as a flat binary file of fixed-size
 * structs written with fwrite() and read with fread():
 *
 *   users.bin     → array of User     structs
 *   tenders.bin   → array of Tender   structs
 *   offers.bin    → array of Offer    structs
 *   contracts.bin → array of Contract structs
 *   audit.log     → append-only text  lines
 *
 * Fixed-size structs mean:
 *   - Record N starts at byte offset N * sizeof(struct)
 *   - Random access by index is O(1) with fseek()
 *   - No parsing needed — direct memcpy from disk to struct
 *
 * ATOMIC WRITES
 * -------------
 * Writing the full file is done atomically:
 *   1. Write to a temp file (path + ".tmp")
 *   2. rename(tmp, path)  ← atomic on POSIX: either old or new, never partial
 *
 * This prevents data corruption from power loss mid-write.
 *
 * SOFT DELETE
 * -----------
 * Records are never physically removed. Instead, the `active` field
 * is set to 0 (soft delete). Readers filter out inactive records.
 * This preserves audit history and avoids rewriting the whole file
 * on every deletion.
 */

/* Data file names (relative to data_dir set by file_io_init) */
#define FIO_USERS_FILE     "users.bin"
#define FIO_TENDERS_FILE   "tenders.bin"
#define FIO_OFFERS_FILE    "offers.bin"
#define FIO_CONTRACTS_FILE "contracts.bin"
#define FIO_AUDIT_FILE     "audit.log"

/* Maximum records per file (prototype limit) */
#define FIO_MAX_USERS    1024
#define FIO_MAX_TENDERS  4096
#define FIO_MAX_OFFERS   8192

/* ── Init ──────────────────────────────────────────────────────── */

/*
 * file_io_init -- create data dir and empty .bin files if missing.
 * Must be called once at startup before any other file_io function.
 * Returns 0 on success, -1 on failure.
 */
int file_io_init(const char *data_dir);

/* ── Users ─────────────────────────────────────────────────────── */
int    fio_user_append  (const User *u);
int    fio_user_find_id (uint32_t id,        User *out);
int    fio_user_find_email(const char *email, User *out);
int    fio_user_find_idno (const char *idno,  User *out);
int    fio_user_update  (const User *u);   /* match by id, overwrite */
size_t fio_user_load_all(User *out, size_t max);

/* ── Tenders ───────────────────────────────────────────────────── */
int    fio_tender_append  (const Tender *t);
int    fio_tender_find_id (uint32_t id, Tender *out);
int    fio_tender_update  (const Tender *t);
size_t fio_tender_load_all(Tender *out, size_t max);  /* active only */

/* ── Offers ────────────────────────────────────────────────────── */
int    fio_offer_append        (const Offer *o);
int    fio_offer_find_id       (uint32_t id, Offer *out);
int    fio_offer_update        (const Offer *o);
size_t fio_offer_load_by_tender(uint32_t tender_id, Offer *out, size_t max);

/* ── Audit log ─────────────────────────────────────────────────── */
/* Append a single line to audit.log (timestamp + message). */
void fio_audit_append(uint32_t user_id, const char *action,
                      uint32_t entity_id, const char *detail);

/* ── ID generation ─────────────────────────────────────────────── */
/* Returns a monotonically increasing ID (epoch seconds + counter). */
uint32_t fio_next_id(void);

#endif /* FILE_IO_H */
