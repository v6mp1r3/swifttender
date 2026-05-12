#ifndef NOTIFY_H
#define NOTIFY_H

#include <stdint.h>
#include "../models/notification.h"
#include "../dsa/queue.h"

/*
 * notify.h -- per-user notification queue management.
 *
 * Wraps dsa/queue.c (circular queue) with a user-indexed table.
 * Each user gets a dedicated CircularQueue slot, found by hashing
 * the user_id:  slot = user_id % NOTIFY_MAX_USERS
 *
 * This is the primary runtime use of the circular queue DSA:
 *   notify_enqueue  -> queue_enqueue  O(1)
 *   notify_drain    -> queue_drain    O(n)
 *
 * Called from:
 *   offer_winner_handler   -> WINNER + LOSER notifications
 *   contract_handler       -> CONTRACT_READY + SIGNED notifications
 */

#define NOTIFY_MAX_USERS 256   /* max concurrent users in prototype */

/* Initialise the global queue table. Call once at startup. */
void notify_init(void);

/* Free all queues. Call at shutdown. */
void notify_cleanup(void);

/*
 * notify_enqueue -- add a notification to a user's circular queue.
 *
 * Parameters:
 *   user_id  : recipient user ID
 *   type     : notification type (NOTIF_WINNER, NOTIF_LOSER, etc.)
 *   tender_id: associated tender (context)
 *   message  : human-readable notification text
 */
void notify_enqueue(uint32_t user_id, NotifType type,
                    uint32_t tender_id, const char *message);

/*
 * notify_drain -- copy and clear all pending notifications for a user.
 *
 * Writes at most `max` Notification structs to `out` (oldest first).
 * Returns number of notifications written.
 */
int notify_drain(uint32_t user_id, Notification *out, int max);

/* Return number of pending notifications for a user. */
int notify_count(uint32_t user_id);

#endif /* NOTIFY_H */
