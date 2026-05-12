/*
 * SwiftTender — main.c
 *
 * Entry point. Sets up the mongoose embedded HTTP server, serves the
 * React frontend as static files, and dispatches all /api/* requests
 * to the router.
 *
 * Build:  make
 * Run:    ./swifttender
 * Server: http://localhost:8000
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "mongoose.h"
#include "router.h"
#include "storage/file_io.h"
#include "utils/auth.h"
#include "handlers/tender_handler.h"
#include "utils/notify.h"

/* ── Configuration ────────────────────────────────────────────── */
#define SERVER_PORT   "http://0.0.0.0:8000"
#define STATIC_DIR    "../frontend/dist"   /* React build output    */
#define DATA_DIR      "./data"             /* binary data files     */

/* ── Global running flag (set to 0 by SIGINT/SIGTERM) ─────────── */
static volatile int s_running = 1;

static void signal_handler(int sig) {
    (void)sig;
    s_running = 0;
}

/*
 * event_handler — called by mongoose for every network event.
 *
 * Routing logic:
 *   /api/*        → router_dispatch()  (business logic in C)
 *   anything else → serve static files from STATIC_DIR (React SPA)
 */
static void event_handler(struct mg_connection *c, int ev,
                           void *ev_data) {
    if (ev != MG_EV_HTTP_MSG) return;

    struct mg_http_message *hm = (struct mg_http_message *) ev_data;

    /* ── CORS preflight (OPTIONS) ─────────────────────────────── */
    if (mg_vcasecmp(&hm->method, "OPTIONS") == 0) {
        mg_http_reply(c, 204,
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET,POST,PATCH,DELETE,OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type,Authorization\r\n",
            "");
        return;
    }

    /* ── API routes ────────────────────────────────────────────── */
    if (mg_http_match_uri(hm, "/api/#")) {
        router_dispatch(c, hm);
        return;
    }

    /* ── Static file serving (React SPA) ──────────────────────── */
    struct mg_http_serve_opts opts = {
        .root_dir = STATIC_DIR,
        .extra_headers =
            "Access-Control-Allow-Origin: *\r\n"
            "Cache-Control: no-cache\r\n"
    };
    mg_http_serve_dir(c, hm, &opts);
}

/* ── Main ─────────────────────────────────────────────────────── */
int main(void) {
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    printf("=========================================\n");
    printf("  SwiftTender — Procurement Platform\n");
    printf("=========================================\n");

    /* Ensure data directory and binary files are initialised */
    if (file_io_init(DATA_DIR) != 0) {
        fprintf(stderr, "[ERROR] Failed to initialise data directory: %s\n",
                DATA_DIR);
        return EXIT_FAILURE;
    }

    /* Initialise session hash table (DSA: hash_table.c) */
    auth_init();

    /* Load tender catalogue into doubly linked list (DSA: linked_list.c) */
    tender_list_init(DATA_DIR);

    /* Initialise per-user notification queues (DSA: queue.c) */
    notify_init();

    /* Initialise mongoose event manager */
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    /* Start HTTP listener */
    struct mg_connection *conn =
        mg_http_listen(&mgr, SERVER_PORT, event_handler, NULL);

    if (conn == NULL) {
        fprintf(stderr, "[ERROR] Cannot listen on %s\n", SERVER_PORT);
        mg_mgr_free(&mgr);
        return EXIT_FAILURE;
    }

    printf("[INFO]  Listening on   %s\n", SERVER_PORT);
    printf("[INFO]  Serving SPA from: %s\n", STATIC_DIR);
    printf("[INFO]  Data directory:   %s\n", DATA_DIR);
    printf("[INFO]  Press Ctrl+C to stop\n\n");

    /* Event loop — polls every 100ms */
    while (s_running) {
        mg_mgr_poll(&mgr, 100);
    }

    printf("\n[INFO]  Shutting down...\n");
    auth_cleanup();
    notify_cleanup();
    mg_mgr_free(&mgr);

    return EXIT_SUCCESS;
}
