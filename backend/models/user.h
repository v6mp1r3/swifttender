#ifndef USER_H
#define USER_H

#include <stdint.h>
#include <time.h>

/* ── User roles ───────────────────────────────────────────── */
typedef enum {
    ROLE_AUTHORITY = 0,   /* contracting authority */
    ROLE_SUPPLIER  = 1    /* economic operator / supplier */
} UserRole;

/* ── Account status ───────────────────────────────────────── */
typedef enum {
    STATUS_PENDING = 0,   /* registered, awaiting admin approval */
    STATUS_ACTIVE  = 1,   /* approved and operational */
    STATUS_BLOCKED = 2    /* suspended */
} UserStatus;

/* ── User struct (persisted in users.bin) ─────────────────── */
typedef struct {
    uint32_t   id;
    char       idno[14];          /* Moldovan fiscal ID — 13 chars + '\0' */
    char       name[128];
    char       email[128];
    char       password_hash[65]; /* SHA-256 hex string */
    UserRole   role;
    UserStatus status;
    char       license_path[256]; /* uploaded company license file path */
    time_t     created_at;
    int        active;            /* soft-delete flag: 1=exists, 0=deleted */
} User;

/* ── Linked-list node wrapper (for in-memory list) ──────────── */
typedef struct UserNode {
    User            data;
    struct UserNode *next;
} UserNode;

#endif /* USER_H */
