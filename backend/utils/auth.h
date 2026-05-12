#ifndef AUTH_H
#define AUTH_H

#include <stdint.h>
#include <stddef.h>
#include "mongoose.h"
#include "dsa/hash_table.h"

/*
 * auth.h -- password hashing, session token management.
 */

#define AUTH_TOKEN_LEN    64
#define AUTH_HASH_LEN     65
#define AUTH_ROUNDS       1000

void     auth_init(void);
void     auth_cleanup(void);
void     auth_hash_password(const char *password, char *out);
int      auth_verify_password(const char *password, const char *stored_hash);
void     auth_generate_token(char *out);
int      auth_token_store(const char *token, uint32_t user_id);
uint32_t auth_token_lookup(const char *token);
void     auth_token_revoke(const char *token);
int      auth_get_bearer(struct mg_http_message *hm, char *out, size_t max);
uint32_t auth_require(struct mg_connection *c, struct mg_http_message *hm);

#endif /* AUTH_H */
