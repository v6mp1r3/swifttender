/*
 * SwiftTender -- router.c
 *
 * Maps incoming HTTP method + URI pairs to handler functions.
 * Uses a static route table (array of Route structs) and a linear
 * scan to find the first match.
 *
 * Pattern matching delegates to mongoose's mg_http_match_uri(),
 * which supports glob-style wildcards:
 *   '#' matches one or more path segments  (/api/tenders/#)
 *   '*' matches a single path component    (/api/auth/*)
 *
 * DSA note: the route table is a compile-time static array.
 * Linear scan is O(n) where n = number of routes (~17). For this
 * scale a hash table would add complexity with no real benefit.
 * The scan happens once per HTTP request.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "router.h"
#include "handlers/auth_handler.h"
#include "handlers/tender_handler.h"
#include "handlers/offer_handler.h"
#include "handlers/contract_handler.h"
#include "handlers/notify_handler.h"
#include "handlers/report_handler.h"

/* ----------------------------------------------------------------
 * Route table -- defines the complete SwiftTender REST API surface.
 * Order matters: more specific patterns must come before wildcards.
 * ---------------------------------------------------------------- */
static const Route s_routes[] = {
    /* Auth */
    { "POST",   "/api/auth/register",           auth_register_handler   },
    { "POST",   "/api/auth/login",              auth_login_handler      },
    { "GET",    "/api/auth/me",                 auth_me_handler         },
    { "POST",   "/api/auth/logout",             auth_logout_handler     },

    /* Tenders */
    { "GET",    "/api/tenders",                 tender_list_handler     },
    { "POST",   "/api/tenders",                 tender_create_handler   },
    { "GET",    "/api/tenders/#",               tender_get_handler      },
    { "PATCH",  "/api/tenders/#",               tender_update_handler   },
    { "DELETE", "/api/tenders/#",               tender_delete_handler   },

    /* Offers (nested under tender) */
    { "GET",    "/api/tenders/#/offers",        offer_list_handler      },
    { "POST",   "/api/tenders/#/offers",        offer_create_handler    },
    { "POST",   "/api/tenders/#/winner",        offer_winner_handler    },

    /* Contracts */
    { "GET",    "/api/tenders/#/contract",      contract_get_handler    },
    { "POST",   "/api/tenders/#/sign",          contract_sign_handler   },
    { "POST",   "/api/tenders/#/documents",     contract_docs_handler   },

    /* Notifications */
    { "GET",    "/api/notifications",           notify_list_handler     },
    { "PATCH",  "/api/notifications/#/read",    notify_read_handler     },

    /* Reports */
    { "GET",    "/api/reports/quarterly",       report_quarterly_handler},

    /* Uploads */
    { "POST",   "/api/uploads",                 upload_handler          },
};

static const int s_route_count =
    (int)(sizeof(s_routes) / sizeof(s_routes[0]));

/* ----------------------------------------------------------------
 * router_dispatch
 * ---------------------------------------------------------------- */
void router_dispatch(struct mg_connection *c, struct mg_http_message *hm) {
    for (int i = 0; i < s_route_count; i++) {
        const Route *r = &s_routes[i];

        /* Check method: "*" matches any method */
        int method_ok = (strcmp(r->method, "*") == 0) ||
                        (mg_vcasecmp(&hm->method, r->method) == 0);
        if (!method_ok) continue;

        /* Check URI pattern */
        if (!mg_http_match_uri(hm, r->pattern)) continue;

        /* Match found -- call handler and return */
        r->handler(c, hm);
        return;
    }

    /* No route matched */
    router_send_error(c, 404, "Route not found");
}

/* ----------------------------------------------------------------
 * router_send_json
 * ---------------------------------------------------------------- */
void router_send_json(struct mg_connection *c, int status,
                      const char *json_fmt, ...) {
    char body[4096];
    va_list ap;
    va_start(ap, json_fmt);
    vsnprintf(body, sizeof(body), json_fmt, ap);
    va_end(ap);

    mg_http_reply(c, status,
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n",
        "%s", body);
}

/* ----------------------------------------------------------------
 * router_send_error
 * ---------------------------------------------------------------- */
void router_send_error(struct mg_connection *c, int status,
                       const char *message) {
    router_send_json(c, status, "{\"error\":\"%s\"}", message);
}

/* ----------------------------------------------------------------
 * router_extract_id
 *
 * Splits the URI by '/' and returns the numeric value of the
 * (segment+1)-th component after the leading '/api/' prefix.
 *
 * Example:
 *   URI: /api/tenders/42/offers
 *   segments (0-based after splitting by '/'):
 *     0: "api"
 *     1: "tenders"
 *     2: "42"      <- segment=2 -> returns 42
 *     3: "offers"
 * ---------------------------------------------------------------- */
long router_extract_id(struct mg_http_message *hm, int segment) {
    /* Copy URI to a mutable buffer */
    char uri[256];
    int len = (int)hm->uri.len < 255 ? (int)hm->uri.len : 255;
    memcpy(uri, hm->uri.buf, (size_t)len);
    uri[len] = '\0';

    char *token = strtok(uri, "/");
    int   idx   = 0;

    while (token != NULL) {
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
