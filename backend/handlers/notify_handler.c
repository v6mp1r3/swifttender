/*
 * notify_handler.c
 * GET /api/notifications, PATCH /api/notifications/:id/read
 *
 * DSA in use: circular queue (dsa/queue.c) via utils/notify.c
 */

#include "notify_handler.h"
#include "../router.h"
#include "../utils/auth.h"
#include "../utils/json.h"
#include "../utils/notify.h"
#include "../models/notification.h"

#include <stdio.h>
#include <string.h>

/* ================================================================
 * GET /api/notifications
 *
 * Drains the current user's circular queue (oldest-first FIFO).
 * Returns all pending notifications and clears the queue.
 * ================================================================ */
void notify_list_handler(struct mg_connection *c,
                          struct mg_http_message *hm) {
    uint32_t user_id = auth_require(c, hm);
    if (!user_id) return;

    Notification notifs[64];

    /* queue_drain via notify_drain: O(n), empties the queue */
    int count = notify_drain(user_id, notifs, 64);

    char body[16384];
    int  n = 0;
    n += snprintf(body + n, sizeof(body) - (size_t)n,
                  "{\"notifications\":[");

    for (int i = 0; i < count; i++) {
        Notification *notif = &notifs[i];

        const char *type_str =
            notif->type == NOTIF_WINNER         ? "WINNER"          :
            notif->type == NOTIF_LOSER          ? "LOSER"           :
            notif->type == NOTIF_NEW_TENDER     ? "NEW_TENDER"      :
            notif->type == NOTIF_CONTRACT_READY ? "CONTRACT_READY"  :
            notif->type == NOTIF_DOC_REQUIRED   ? "DOC_REQUIRED"    : "SIGNED";

        char escaped[512];
        json_escape(notif->message, escaped, sizeof(escaped));

        n += snprintf(body + n, sizeof(body) - (size_t)n,
            "{\"id\":%u,\"type\":\"%s\",\"tenderId\":%u,"
            "\"message\":\"%s\",\"createdAt\":%ld},",
            notif->id, type_str, notif->tender_id,
            escaped, (long)notif->created_at);
    }
    if (count > 0 && body[n - 1] == ',') n--;
    n += snprintf(body + n, sizeof(body) - (size_t)n,
                  "],\"count\":%d}", count);

    router_send_json(c, 200, "%s", body);
}

/* ================================================================
 * PATCH /api/notifications/:id/read
 *
 * Notifications are drained (marked read) on GET. This endpoint
 * exists for explicit read marking if needed by the frontend.
 * ================================================================ */
void notify_read_handler(struct mg_connection *c,
                          struct mg_http_message *hm) {
    uint32_t user_id = auth_require(c, hm);
    if (!user_id) return;
    (void)hm;
    router_send_json(c, 200, "{\"message\":\"Marked as read\"}");
}
