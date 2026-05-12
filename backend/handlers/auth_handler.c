/*
 * auth_handler.c -- POST /api/auth/register|login, GET /me, POST /logout
 */

#include "auth_handler.h"
#include "router.h"
#include "utils/auth.h"
#include "utils/json.h"
#include "storage/file_io.h"
#include "storage/upload.h"
#include "models/user.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

/* ----------------------------------------------------------------
 * user_to_json -- serialise a User struct to a JSON object string.
 * Omits password_hash and license_path (sensitive/internal fields).
 * ---------------------------------------------------------------- */
static int user_to_json(const User *u, char *buf, int max) {
    int n = 0;
    n += snprintf(buf + n, (size_t)(max - n), "{");
    n += jb_long (buf + n, max - n, "id",     (long)u->id);
    n += jb_str  (buf + n, max - n, "name",   u->name);
    n += jb_str  (buf + n, max - n, "email",  u->email);
    n += jb_str  (buf + n, max - n, "idno",   u->idno);
    n += jb_str  (buf + n, max - n, "role",
                  u->role == ROLE_AUTHORITY ? "AUTHORITY" : "SUPPLIER");
    n += jb_str  (buf + n, max - n, "status",
                  u->status == STATUS_ACTIVE  ? "ACTIVE"  :
                  u->status == STATUS_PENDING ? "PENDING" : "BLOCKED");
    n += jb_long (buf + n, max - n, "createdAt", (long)u->created_at);
    /* Remove trailing comma before closing brace */
    if (n > 0 && buf[n - 1] == ',') n--;
    n += snprintf(buf + n, (size_t)(max - n), "}");
    return n;
}

/* ================================================================
 * POST /api/auth/register
 *
 * Body (JSON):
 *   { "name", "email", "password", "idno", "role" }
 *
 * Flow:
 *   1. Parse and validate required fields.
 *   2. Check email and IDNO are not already registered.
 *   3. Hash password.
 *   4. Persist User with STATUS_ACTIVE (prototype: skip admin review).
 *   5. Generate session token, store in hash table.
 *   6. Return { token, user }.
 * ================================================================ */
void auth_register_handler(struct mg_connection *c,
                            struct mg_http_message *hm) {
    char name[128], email[128], password[128], idno[14], role_str[16];

    /* Parse required fields */
    if (json_get_str(hm->body, "$.name",     name,     sizeof(name))     != 0 ||
        json_get_str(hm->body, "$.email",    email,    sizeof(email))    != 0 ||
        json_get_str(hm->body, "$.password", password, sizeof(password)) != 0 ||
        json_get_str(hm->body, "$.idno",     idno,     sizeof(idno))     != 0 ||
        json_get_str(hm->body, "$.role",     role_str, sizeof(role_str)) != 0) {
        router_send_error(c, 400,
            "Required fields: name, email, password, idno, role");
        return;
    }

    /* Validate role */
    UserRole role;
    if (strncmp(role_str, "AUTHORITY", 9) == 0)
        role = ROLE_AUTHORITY;
    else if (strncmp(role_str, "SUPPLIER", 8) == 0)
        role = ROLE_SUPPLIER;
    else {
        router_send_error(c, 400, "role must be AUTHORITY or SUPPLIER");
        return;
    }

    /* Check for duplicate email */
    User existing;
    if (fio_user_find_email(email, &existing) == 0) {
        router_send_error(c, 409, "Email already registered");
        return;
    }

    /* Check for duplicate IDNO */
    if (fio_user_find_idno(idno, &existing) == 0) {
        router_send_error(c, 409, "IDNO already registered");
        return;
    }

    /* Build User struct */
    User u;
    memset(&u, 0, sizeof(u));
    u.id         = fio_next_id();
    u.role       = role;
    u.status     = STATUS_ACTIVE;   /* prototype: instant activation */
    u.created_at = time(NULL);
    u.active     = 1;

    strncpy(u.name,  name,  sizeof(u.name)  - 1);
    strncpy(u.email, email, sizeof(u.email) - 1);
    strncpy(u.idno,  idno,  sizeof(u.idno)  - 1);
    auth_hash_password(password, u.password_hash);

    /* Persist */
    if (fio_user_append(&u) != 0) {
        router_send_error(c, 500, "Failed to save user");
        return;
    }

    /* Generate session token and store in hash table */
    char token[AUTH_TOKEN_LEN + 1];
    auth_generate_token(token);
    auth_token_store(token, u.id);

    fio_audit_append(u.id, "REGISTER", u.id, u.email);

    /* Build response */
    char user_json[512];
    user_to_json(&u, user_json, sizeof(user_json));

    router_send_json(c, 201,
        "{\"token\":\"%s\",\"user\":%s}", token, user_json);
}

/* ================================================================
 * POST /api/auth/login
 *
 * Body: { "email", "password" }
 * Returns: { token, user }
 * ================================================================ */
void auth_login_handler(struct mg_connection *c,
                         struct mg_http_message *hm) {
    char email[128], password[128];

    if (json_get_str(hm->body, "$.email",    email,    sizeof(email))    != 0 ||
        json_get_str(hm->body, "$.password", password, sizeof(password)) != 0) {
        router_send_error(c, 400, "Required fields: email, password");
        return;
    }

    /* Look up user by email */
    User u;
    if (fio_user_find_email(email, &u) != 0) {
        /* Generic message to avoid user enumeration */
        router_send_error(c, 401, "Invalid email or password");
        return;
    }

    /* Check account status */
    if (u.status == STATUS_BLOCKED) {
        router_send_error(c, 403, "Account is blocked");
        return;
    }

    /* Verify password */
    if (!auth_verify_password(password, u.password_hash)) {
        router_send_error(c, 401, "Invalid email or password");
        return;
    }

    /* Issue session token -- stored in hash table (O(1) future lookups) */
    char token[AUTH_TOKEN_LEN + 1];
    auth_generate_token(token);
    auth_token_store(token, u.id);

    fio_audit_append(u.id, "LOGIN", u.id, u.email);

    char user_json[512];
    user_to_json(&u, user_json, sizeof(user_json));

    router_send_json(c, 200,
        "{\"token\":\"%s\",\"user\":%s}", token, user_json);
}

/* ================================================================
 * GET /api/auth/me
 *
 * Header: Authorization: Bearer <token>
 * Returns: { user }
 * ================================================================ */
void auth_me_handler(struct mg_connection *c,
                      struct mg_http_message *hm) {
    /* auth_require: validates token via hash table lookup, sends 401 on fail */
    uint32_t user_id = auth_require(c, hm);
    if (!user_id) return;   /* 401 already sent */

    User u;
    if (fio_user_find_id(user_id, &u) != 0) {
        router_send_error(c, 404, "User not found");
        return;
    }

    char user_json[512];
    user_to_json(&u, user_json, sizeof(user_json));
    router_send_json(c, 200, "{\"user\":%s}", user_json);
}

/* ================================================================
 * POST /api/auth/logout
 *
 * Header: Authorization: Bearer <token>
 * Removes token from hash table.
 * ================================================================ */
void auth_logout_handler(struct mg_connection *c,
                          struct mg_http_message *hm) {
    char token[AUTH_TOKEN_LEN + 1];
    if (auth_get_bearer(hm, token, sizeof(token)) == 0) {
        uint32_t uid = auth_token_lookup(token);
        auth_token_revoke(token);
        fio_audit_append(uid, "LOGOUT", uid, "-");
    }
    router_send_json(c, 200, "{\"message\":\"Logged out\"}");
}
