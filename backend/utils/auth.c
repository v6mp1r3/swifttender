/*
 * auth.c -- password hashing and session token management.
 *
 * The global session hash table (s_sessions) is the primary runtime
 * use of the hash table DSA from dsa/hash_table.c. Every authenticated
 * request calls auth_token_lookup() -> ht_lookup() -> O(1) lookup.
 */

#include "auth.h"
#include "../router.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── Global session hash table ──────────────────────────────────── */
static HashTable *s_sessions = NULL;

void auth_init(void) {
    s_sessions = ht_create();
    if (!s_sessions)
        fprintf(stderr, "[auth] Failed to create session table\n");
    else
        printf("[auth] Session table ready\n");
}

void auth_cleanup(void) {
    ht_destroy(s_sessions);
    s_sessions = NULL;
}

/* ── Password hashing ───────────────────────────────────────────── */

/*
 * djb2_round -- one pass of the djb2 algorithm over a string.
 * Returns the resulting hash value.
 */
static unsigned long djb2_round(const char *str, unsigned long seed) {
    unsigned long hash = seed;
    int c;
    while ((c = (unsigned char)*str++))
        hash = ((hash << 5) + hash) ^ (unsigned long)c;
    return hash;
}

/*
 * auth_hash_password -- iterative djb2 password hashing.
 *
 * Runs AUTH_ROUNDS (1000) iterations of djb2, each feeding the
 * previous output back as the seed. This makes brute-force
 * significantly slower than a single hash pass.
 *
 * NOTE: For production, use bcrypt or Argon2. This implementation
 * is intentionally simple for a course prototype and is documented
 * as a limitation in the technical requirements section.
 */
void auth_hash_password(const char *password, char *out) {
    unsigned long hash = 5381;
    for (int i = 0; i < AUTH_ROUNDS; i++) {
        hash = djb2_round(password, hash);
    }
    /* Format as 16-char hex string (padded to AUTH_HASH_LEN) */
    snprintf(out, AUTH_HASH_LEN, "%016lx%016lx%016lx%016lx",
             hash, hash ^ 0xDEADBEEF, hash * 31, hash + 0xCAFEBABE);
}

int auth_verify_password(const char *password, const char *stored_hash) {
    char computed[AUTH_HASH_LEN];
    auth_hash_password(password, computed);
    return strncmp(computed, stored_hash, AUTH_HASH_LEN) == 0 ? 1 : 0;
}

/* ── Token generation ───────────────────────────────────────────── */

/*
 * auth_generate_token -- create a unique 64-char hex session token.
 *
 * Combines: current time, a static counter, and a pseudo-random
 * value derived from the counter. Not cryptographically secure, but
 * unique and unpredictable enough for a course prototype.
 */
void auth_generate_token(char *out) {
    static unsigned long counter = 0;
    counter++;

    unsigned long a = (unsigned long)time(NULL);
    unsigned long b = counter;
    unsigned long c = djb2_round("swifttender_salt", a ^ b);
    unsigned long d = djb2_round("moldova_procurement", b ^ c);

    snprintf(out, AUTH_TOKEN_LEN + 1,
             "%016lx%016lx%016lx%016lx", a, b, c, d);
}

/* ── Session table operations ───────────────────────────────────── */

/*
 * auth_token_store -- store token -> user_id in the hash table.
 *
 * We cast user_id to (void*) to store it as the hash table value.
 * On lookup we cast back. This avoids a separate allocation per
 * session and keeps the hash table generic (void* values).
 */
int auth_token_store(const char *token, uint32_t user_id) {
    if (!s_sessions) return -1;
    return ht_insert(s_sessions, token, (void*)(uintptr_t)user_id);
}

uint32_t auth_token_lookup(const char *token) {
    if (!s_sessions || !token) return 0;
    void *val = ht_lookup(s_sessions, token);
    return val ? (uint32_t)(uintptr_t)val : 0;
}

void auth_token_revoke(const char *token) {
    if (s_sessions && token) ht_delete(s_sessions, token);
}

/* ── HTTP helpers ───────────────────────────────────────────────── */

int auth_get_bearer(struct mg_http_message *hm, char *out, size_t max) {
    /* Look for "Authorization: Bearer <token>" header */
    struct mg_str auth_header = mg_http_get_header_var(
        *hm, mg_str("Authorization"), mg_str("Bearer"));

    /* Fallback: check the full Authorization header */
    if (auth_header.len == 0) {
        struct mg_str h = mg_http_get_header(hm, "Authorization");
        if (h.len == 0) return -1;
        /* Skip "Bearer " prefix (7 chars) */
        if (h.len <= 7) return -1;
        auth_header.buf = h.buf + 7;
        auth_header.len = h.len - 7;
    }

    if (auth_header.len == 0 || auth_header.len >= max) return -1;
    memcpy(out, auth_header.buf, auth_header.len);
    out[auth_header.len] = '\0';
    return 0;
}

uint32_t auth_require(struct mg_connection *c, struct mg_http_message *hm) {
    char token[AUTH_TOKEN_LEN + 1];
    if (auth_get_bearer(hm, token, sizeof(token)) != 0) {
        router_send_error(c, 401, "Missing Authorization header");
        return 0;
    }
    uint32_t user_id = auth_token_lookup(token);
    if (user_id == 0) {
        router_send_error(c, 401, "Invalid or expired session token");
        return 0;
    }
    return user_id;
}
