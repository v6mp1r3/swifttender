/*
 * ==================================================================
 *  SwiftTender -- Circular Queue Implementation
 *  dsa/queue.c
 * ==================================================================
 */

#include "queue.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ----------------------------------------------------------------
 * queue_create -- heap-allocate and zero-initialise a queue.
 *
 * Zero-init sets head=0, tail=0, count=0 which is the correct
 * empty state without an explicit assignment loop.
 * ---------------------------------------------------------------- */
CircularQueue *queue_create(void) {
    CircularQueue *q = calloc(1, sizeof(CircularQueue));
    return q;   /* NULL on allocation failure */
}

/* ----------------------------------------------------------------
 * queue_destroy -- free the queue struct.
 * ---------------------------------------------------------------- */
void queue_destroy(CircularQueue *q) {
    free(q);
}

/* ----------------------------------------------------------------
 * queue_enqueue -- add a notification at the tail in O(1).
 *
 * The circular wrap-around uses the modulo operator:
 *
 *   tail_next = (tail + 1) % QUEUE_CAPACITY
 *
 * This keeps tail within [0, QUEUE_CAPACITY-1] automatically.
 *
 * Visual example (QUEUE_CAPACITY = 6, count = 4):
 *
 *   Before:  [N0, N1, N2, N3,  _,  _]
 *              ^               ^
 *             head=0         tail=4
 *
 *   Enqueue N4:
 *     data[tail=4] = N4
 *     tail = (4+1) % 6 = 5
 *     count = 5
 *
 *   After:   [N0, N1, N2, N3, N4,  _]
 *              ^                   ^
 *             head=0             tail=5
 *
 * FULL CASE (overwrite oldest):
 * If count == QUEUE_CAPACITY, we overwrite data[tail] AND advance
 * head by 1 (the oldest entry is sacrificed). This is the bounded-
 * buffer design: a new urgent notification is never dropped.
 *
 *   Before:  [N0, N1, N2, N3, N4, N5]  count=6, head=0, tail=0
 *
 *   Enqueue N6 (overwrites N0 at index 0):
 *     data[0] = N6
 *     tail = (0+1) % 6 = 1
 *     head = (0+1) % 6 = 1   <- advance head, N0 is dropped
 *     count stays at 6
 *
 *   After:   [N6, N1, N2, N3, N4, N5]  count=6, head=1, tail=1
 * ---------------------------------------------------------------- */
void queue_enqueue(CircularQueue *q, const Notification *notif) {
    if (!q || !notif) return;

    if (q->count == QUEUE_CAPACITY) {
        /* Queue full: overwrite oldest entry, advance head */
        memcpy(&q->data[q->tail], notif, sizeof(Notification));
        q->tail = (q->tail + 1) % QUEUE_CAPACITY;
        q->head = (q->head + 1) % QUEUE_CAPACITY;  /* oldest lost */
    } else {
        /* Normal case: write at tail and advance it */
        memcpy(&q->data[q->tail], notif, sizeof(Notification));
        q->tail = (q->tail + 1) % QUEUE_CAPACITY;
        q->count++;
    }
}

/* ----------------------------------------------------------------
 * queue_dequeue -- remove and return the oldest notification O(1).
 *
 * Reads from head, then advances head with wrap-around.
 *
 * Visual example (continuing from above, count=5):
 *
 *   Before:  [N0, N1, N2, N3, N4,  _]   head=0, tail=5
 *
 *   Dequeue:
 *     *out = data[head=0]  (= N0)
 *     head = (0+1) % 6 = 1
 *     count = 4
 *
 *   After:   [ _ , N1, N2, N3, N4,  _]   head=1, tail=5
 *                  ^               ^
 * ---------------------------------------------------------------- */
int queue_dequeue(CircularQueue *q, Notification *out) {
    if (!q || !out || q->count == 0) return 0;

    memcpy(out, &q->data[q->head], sizeof(Notification));
    q->head = (q->head + 1) % QUEUE_CAPACITY;
    q->count--;
    return 1;
}

/* ----------------------------------------------------------------
 * queue_peek -- read head without removing it.
 * ---------------------------------------------------------------- */
int queue_peek(const CircularQueue *q, Notification *out) {
    if (!q || !out || q->count == 0) return 0;
    memcpy(out, &q->data[q->head], sizeof(Notification));
    return 1;
}

/* ----------------------------------------------------------------
 * Accessors -- all O(1).
 * ---------------------------------------------------------------- */
int queue_is_empty(const CircularQueue *q) {
    return (!q || q->count == 0) ? 1 : 0;
}

int queue_is_full(const CircularQueue *q) {
    return (q && q->count == QUEUE_CAPACITY) ? 1 : 0;
}

int queue_count(const CircularQueue *q) {
    return q ? q->count : 0;
}

/* ----------------------------------------------------------------
 * queue_drain -- copy all entries oldest-first and empty the queue.
 *
 * Used by GET /api/notifications to send all pending notifications
 * to the client and mark them as delivered.
 *
 * Simply calls queue_dequeue repeatedly: each call is O(1),
 * total is O(n) for n notifications.
 * ---------------------------------------------------------------- */
int queue_drain(CircularQueue *q, Notification *out, int max) {
    if (!q || !out || max <= 0) return 0;

    int written = 0;
    while (written < max && !queue_is_empty(q)) {
        queue_dequeue(q, &out[written]);
        written++;
    }
    return written;
}
