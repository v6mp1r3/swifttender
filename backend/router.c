/*
 * SwiftTender -- router.c
 * Compatible with any mongoose 7.x version.
 * Uses strcasecmp + custom uri_match instead of removed mongoose functions.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>

#include "router.h"
#include "handlers/auth_handler.h"
#include "handlers/tender_handler.h"
#include "handlers/offer_handler.h"
#include "handlers/contract_handler.h"
#include "handlers/notify_handler.h"
#include "handlers/report_handler.h"

/*
 * uri_match -- simple glob matcher.
 * Wildcard: * matches one path segment (no slashes).
 * Exact patterns match literally.
 */
static int uri_match(const char *pattern, const char *uri) {
    while (*pattern && *uri) {
        if (*pattern == '*') {
            pattern++;
            while (*uri && *uri != '/') uri++;
        } else if (*pattern == *uri) {
            pattern++;
            uri++;
        } else {
            return 0;
        }
    }
    return (*pattern == '\0' && *uri == '\0');
}

/*
 * Route table.
 * ORDER MATTERS: more specific patterns before wildcards.
 * "/api/tenders/X/offers" must come before "/api/tenders/X"
 */
static const Route s_routes[] = {
    { "POST",   "/api/auth/register",          auth_register_handler    },
    { "POST",   "/api/auth/login",             auth_login_handler       },
    { "GET",    "/api/auth/me",                auth_me_handler          },
    { "POST",   "/api/auth/logout",            auth_logout_handler      },

    { "GET",    "/api/tenders",                tender_list_handler      },
    { "POST",   "/api/tenders",                tender_create_handler    },

    { "GET",    "/api/tenders/*/offers",       offer_list_handler       },
    { "POST",   "/api/tenders/*/offers",       offer_create_handler     },
    { "POST",   "/api/tenders/*/winner",       offer_winner_handler     },
    { "GET",    "/api/tenders/*/contract",     contract_get_handler     },
    { "POST",   "/api/tenders/*/sign",         contract_sign_handler    },
    { "POST",   "/api/tenders/*/documents",    contract_docs_handler    },

    { "GET",    "/api/tenders/*",              tender_get_handler       },
    { "PATCH",  "/api/tenders/*",              tender_update_handler    },
    { "DELETE", "/api/tenders/*",              tender_delete_handler    },

    { "GET",    "/api/notifications",          notify_list_handler      },
    { "PATCH",  "/api/notifications/*",        notify_read_handler      },

    { "GET",    "/api/reports/quarterly",      report_quarterly_handler },
    { "POST",   "/api/uploads",                upload_handler           },
};

static const int s_route_count =
    (int)(sizeof(s_routes) / sizeof(s_routes[0]));

void router_dispatch(struct mg_connection *c, struct mg_http_message *hm) {
    char method[16] = {0};
    char uri[512]   = {0};
    size_t mlen = hm->method.len < 15  ? hm->method.len : 15;
    size_t ulen = hm->uri.len    < 511 ? hm->uri.len    : 511;
    memcpy(method, hm->method.buf, mlen);
    memcpy(uri,    hm->uri.buf,    ulen);

    fprintf(stderr, "[router] %s %s\n", method, uri);

    for (int i = 0; i < s_route_count; i++) {
        const Route *r = &s_routes[i];
        if (strcasecmp(r->method, method) != 0) continue;
        if (!uri_match(r->pattern, uri)) continue;
        fprintf(stderr, "[router] -> %s %s\n", r->method, r->pattern);
        r->handler(c, hm);
        return;
    }
    fprintf(stderr, "[router] 404: %s %s\n", method, uri);
    router_send_error(c, 404, "Route not found");
}

void router_send_json(struct mg_connection *c, int status,
                      const char *json_fmt, ...) {
    static char body[131072];
    va_list ap;
    va_start(ap, json_fmt);
    vsnprintf(body, sizeof(body), json_fmt, ap);
    va_end(ap);
    mg_http_reply(c, status,
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET,POST,PATCH,DELETE,OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type,Authorization\r\n",
        "%s", body);
}

void router_send_error(struct mg_connection *c, int status,
                       const char *message) {
    char safe[512] = {0};
    const char *src = message ? message : "error";
    int j = 0;
    for (int i = 0; src[i] && j < 510; i++) {
        if (src[i] == '"' || src[i] == '\\') safe[j++] = '\\';
        safe[j++] = src[i];
    }
    router_send_json(c, status, "{\"error\":\"%s\"}", safe);
}

long router_extract_id(struct mg_http_message *hm, int segment) {
    char uri[256] = {0};
    size_t ulen = hm->uri.len < 255 ? hm->uri.len : 255;
    memcpy(uri, hm->uri.buf, ulen);
    char *token = strtok(uri, "/");
    int   idx   = 0;
    while (token) {
        if (idx == segment) {
            char *end;
            long id = strtol(token, &end, 10);
            if (*end == '\0' && id > 0) return id;
            return -1;
        }
        token = strtok(NULL, "/");
        idx++;
    }
    return -1;
}
