#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>
#include "models/notification.h"

/*
 * ==================================================================
 *  SwiftTender -- Circular Queue (Notification System)
 *  dsa/queue.h
 * ==================================================================
 *
 * PURPOSE
 * -------
 * Stores pending notifications for each user as a FIFO queue.
 * Events that trigger enqueue:
 *   - Winner selected      → NOTIF_WINNER  enqueued for winning supplier
 *   - Winner selected      → NOTIF_LOSER   enqueued for all other suppliers
 *   - New tender published → NOTIF_NEW_TENDER for all active suppliers
 *   - Contract ready       → NOTIF_CONTRACT_READY for winner
 *   - Both parties signed  → NOTIF_SIGNED for both
 *
 * WHY A CIRCULAR QUEUE (not a linked list or simple array)?
 * ----------------------------------------------------------
 * Notifications follow strict FIFO semantics: oldest first.
 * A circular queue gives O(1) enqueue AND dequeue with no shifting:
 *
 *   Simple array queue:
 *     enqueue: O(1) append at tail
 *     dequeue: O(n) shift all remaining elements left  ← bad
 *
 *   Circular queue:
 *     enqueue: O(1) advance tail pointer (with wrap-around)
 *     dequeue: O(1) advance head pointer (with wrap-around)
 *
 * The "circular" trick: instead of shifting data, we let head and
 * tail pointers chase each other around a fixed-size array:
 *
 *   [_, _, N3, N4, N5, _, _, _]
 *           ^           ^
 *          head        tail
 *
 * When tail reaches the end of the array, it wraps to index 0.
 * The array is "circular" in the sense that index (i+1) % capacity
 * follows index i, and index 0 follows index (capacity-1).
 *
 * FULL vs EMPTY DISTINCTION
 * -------------------------
 * Both empty and full states have head == tail if we only use
 * pointers. We resolve this by maintaining an explicit `count`
 * field: empty iff count==0, full iff count==capacity.
 *
 * FIXED CAPACITY
 * --------------
 * Each user queue holds at most QUEUE_CAPACITY notifications.
 * If the queue is full when a new notification arrives, the oldest
 * entry is overwritten (oldest notifications are least relevant).
 * This is an intentional design choice for a bounded buffer.
 *
 * OPERATIONS
 * ----------
 *   queue_enqueue   O(1)  -- add notification at tail
 *   queue_dequeue   O(1)  -- remove notification from head
 *   queue_peek      O(1)  -- read head without removing
 *   queue_is_empty  O(1)
 *   queue_is_full   O(1)
 *   queue_count     O(1)
 */

#define QUEUE_CAPACITY 64   /* max pending notifications per user */

typedef struct {
    Notification data[QUEUE_CAPACITY];  /* fixed-size circular buffer */
    int          head;                  /* index of oldest item       */
    int          tail;                  /* index of next free slot    */
    int          count;                 /* number of live items       */
} CircularQueue;

/* Lifecycle */
CircularQueue *queue_create(void);
void           queue_destroy(CircularQueue *q);

/*
 * queue_enqueue -- add a notification at the tail.
 *
 * If queue is full, overwrites the oldest entry (head advances).
 * Always succeeds (no return value needed).
 */
void queue_enqueue(CircularQueue *q, const Notification *notif);

/*
 * queue_dequeue -- remove and return the oldest notification.
 *
 * Returns 1 and writes to *out on success.
 * Returns 0 if the queue is empty (*out unchanged).
 */
int queue_dequeue(CircularQueue *q, Notification *out);

/* Read the oldest notification without removing it. Returns 1 on
 * success, 0 if empty. */
int queue_peek(const CircularQueue *q, Notification *out);

int    queue_is_empty(const CircularQueue *q);
int    queue_is_full (const CircularQueue *q);
int    queue_count   (const CircularQueue *q);

/*
 * queue_drain -- copy all notifications into a caller-supplied array
 * (oldest first) and empty the queue.
 * Returns number of entries written.
 */
int queue_drain(CircularQueue *q, Notification *out, int max);

#endif /* QUEUE_H */
