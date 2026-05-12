#ifndef AUTH_H
#define AUTH_H

#include <stdint.h>
#include "mongoose.h"
#include "../dsa/hash_table.h"

/*
 * auth.h -- password hashing, session token management.
 *
 * Session tokens are stored in a global HashTable (from dsa/hash_table.c)
 * mapping token string -> user_id (as a uintptr_t cast to void*).
 * This is the primary use of the hash table DSA in the live system:
 * every authenticated API request calls auth_token_lookup() which
 * hits the hash table for O(1) auth verification.
 *
 * Password hashing uses multiple rounds of djb2. For a production
 * system, bcrypt or Argon2 would be required. This is clearly noted
 * for the report's limitations section.
 */

#define AUTH_TOKEN_LEN    64    /* hex characters in a session token  */
#define AUTH_HASH_LEN     65    /* 64 hex chars + null terminator     */
#define AUTH_ROUNDS       1000  /* djb2 iteration rounds for hashing  */

/* Initialise the global session hash table. Call once at startup. */
void auth_init(void);

/* Free the global session hash table. Call at shutdown. */
void auth_cleanup(void);

/*
 * auth_hash_password -- hash a plaintext password into a hex string.
 *
 * Uses AUTH_ROUNDS iterations of djb2 to slow down brute-force.
 * out must be at least AUTH_HASH_LEN bytes.
 */
void auth_hash_password(const char *password, char *out);

/*
 * auth_verify_password -- verify plaintext against a stored hash.
 * Returns 1 if matching, 0 if not.
 */
int auth_verify_password(const char *password, const char *stored_hash);

/*
 * auth_generate_token -- generate a unique session token string.
 * out must be at least AUTH_TOKEN_LEN + 1 bytes.
 */
void auth_generate_token(char *out);

/*
 * auth_token_store -- insert token -> user_id into the session table.
 * Returns 0 on success, -1 on failure.
 */
int auth_token_store(const char *token, uint32_t user_id);

/*
 * auth_token_lookup -- find the user_id for a session token.
 * Returns user_id on success, 0 if token not found.
 */
uint32_t auth_token_lookup(const char *token);

/*
 * auth_token_revoke -- remove a token from the session table.
 */
void auth_token_revoke(const char *token);

/*
 * auth_get_bearer -- extract the Bearer token from an HTTP request.
 * Reads the Authorization: Bearer <token> header.
 * Returns 0 on success (token written to out), -1 if header missing.
 */
int auth_get_bearer(struct mg_http_message *hm,
                    char *out, size_t max);

/*
 * auth_require -- middleware: validate token and return user_id.
 * Sends 401 JSON response and returns 0 if token is invalid.
 * Returns user_id (> 0) if valid.
 */
uint32_t auth_require(struct mg_connection *c,
                      struct mg_http_message *hm);

#endif /* AUTH_H */
