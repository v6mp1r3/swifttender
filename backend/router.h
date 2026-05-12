#ifndef ROUTER_H
#define ROUTER_H

#include "mongoose.h"

/*
 * router.h -- URL pattern -> handler dispatch
 *
 * router_dispatch() inspects the incoming HTTP method and URI,
 * finds the first matching entry in the route table, and calls
 * the corresponding handler function.
 *
 * If no route matches, responds with 404 JSON.
 */

/* Function pointer type for all handler functions */
typedef void (*HandlerFn)(struct mg_connection *c,
                          struct mg_http_message *hm);

/* A single entry in the static route table */
typedef struct {
    const char *method;   /* "GET", "POST", "PATCH", "DELETE", or "*" */
    const char *pattern;  /* mongoose URI pattern e.g. "/api/tenders/#" */
    HandlerFn   handler;
} Route;

/* Main dispatch function - called from main.c for every /api/... request */
void router_dispatch(struct mg_connection *c, struct mg_http_message *hm);

/* Send a JSON response with standard CORS headers */
void router_send_json(struct mg_connection *c, int status,
                      const char *json_fmt, ...);

/* Send {"error": "<message>"} */
void router_send_error(struct mg_connection *c, int status,
                       const char *message);

/* Extract numeric ID from URI segment (0-based after /api/) */
/* e.g. /api/tenders/42/offers -> segment=1 -> returns 42 */
long router_extract_id(struct mg_http_message *hm, int segment);

#endif /* ROUTER_H */
