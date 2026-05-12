/*
 * notify.c -- per-user notification queue management.
 *
 * DSA: circular queue (dsa/queue.c)
 * Each user slot holds one CircularQueue (QUEUE_CAPACITY=64 entries).
 * Slot assignment: user_id % NOTIFY_MAX_USERS (simple hash).
 */

#include "notify.h"
#include "../storage/file_io.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* ── Global queue table ─────────────────────────────────────────── */
static CircularQueue s_queues[NOTIFY_MAX_USERS];
static int           s_initialised = 0;

void notify_init(void) {
    memset(s_queues, 0, sizeof(s_queues));
    s_initialised = 1;
    printf("[notify] Notification queues ready (%d slots)\n",
           NOTIFY_MAX_USERS);
}

void notify_cleanup(void) {
    /* Static array -- nothing to free */
    s_initialised = 0;
}

/*
 * slot -- map user_id to a queue index.
 * Simple modulo hash: O(1), deterministic, no collisions to resolve
 * because each slot is independent (users don't share queues).
 */
static int slot(uint32_t user_id) {
    return (int)(user_id % (uint32_t)NOTIFY_MAX_USERS);
}

void notify_enqueue(uint32_t user_id, NotifType type,
                    uint32_t tender_id, const char *message) {
    if (!s_initialised) return;

    Notification n;
    memset(&n, 0, sizeof(n));
    n.id        = fio_next_id();
    n.user_id   = user_id;
    n.type      = type;
    n.tender_id = tender_id;
    n.is_read   = 0;
    n.created_at= time(NULL);

    strncpy(n.message, message ? message : "", sizeof(n.message) - 1);

    /* queue_enqueue: O(1) circular buffer insert */
    queue_enqueue(&s_queues[slot(user_id)], &n);
}

int notify_drain(uint32_t user_id, Notification *out, int max) {
    if (!s_initialised || !out || max <= 0) return 0;
    /* queue_drain: O(n) dequeue all entries oldest-first */
    return queue_drain(&s_queues[slot(user_id)], out, max);
}

int notify_count(uint32_t user_id) {
    if (!s_initialised) return 0;
    return queue_count(&s_queues[slot(user_id)]);
}
