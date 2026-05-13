/*
 * SwiftTender -- main.c
 * Entry point. mongoose HTTP server and event loop.
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

#define DEFAULT_PORT "8000"
#define STATIC_DIR   "../frontend/dist"
#define DATA_DIR     "./data"

static volatile int s_running = 1;
static void signal_handler(int sig) { (void)sig; s_running = 0; }

static void event_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev != MG_EV_HTTP_MSG) return;
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;

    /* CORS preflight */
    if (hm->method.len == 7 &&
        memcmp(hm->method.buf, "OPTIONS", 7) == 0) {
        mg_http_reply(c, 204,
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET,POST,PATCH,DELETE,OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type,Authorization\r\n",
            "");
        return;
    }

    /* Route /api/ prefix to our router */
    if (hm->uri.len >= 5 && memcmp(hm->uri.buf, "/api/", 5) == 0) {
        router_dispatch(c, hm);
        return;
    }

    /* Serve React SPA static files */
    struct mg_http_serve_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.root_dir = static_dir;
    opts.extra_headers =
        "Access-Control-Allow-Origin: *\r\n"
        "Cache-Control: no-cache\r\n";
    mg_http_serve_dir(c, hm, &opts);
}

int main(void) {
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    printf("=========================================\n");
    printf("  SwiftTender -- Procurement Platform\n");
    printf("=========================================\n");

    if (file_io_init(DATA_DIR) != 0) {
        fprintf(stderr, "[ERROR] Failed to init data directory\n");
        return EXIT_FAILURE;
    }

    auth_init();
    tender_list_init(DATA_DIR);
    notify_init();

    /* Read config from environment variables (for cloud deployment) */
    const char *static_dir = getenv("STATIC_DIR");
    if (!static_dir) static_dir = "../frontend/dist";  /* local dev fallback */

    const char *port_env = getenv("PORT");
    char server_url[64];
    snprintf(server_url, sizeof(server_url), "http://0.0.0.0:%s",
             port_env ? port_env : DEFAULT_PORT);

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    struct mg_connection *conn =
        mg_http_listen(&mgr, server_url, event_handler, NULL);
    if (!conn) {
        fprintf(stderr, "[ERROR] Cannot listen on %s\n", SERVER_PORT);
        mg_mgr_free(&mgr);
        return EXIT_FAILURE;
    }

    printf("[INFO]  Listening on   %s\n", server_url);
    printf("[INFO]  Serving SPA:   %s\n", static_dir);
    printf("[INFO]  Data dir:      %s\n", DATA_DIR);
    printf("[INFO]  Press Ctrl+C to stop\n\n");

    while (s_running) mg_mgr_poll(&mgr, 100);

    printf("\n[INFO]  Shutting down...\n");
    auth_cleanup();
    notify_cleanup();
    mg_mgr_free(&mgr);
    return EXIT_SUCCESS;
}
