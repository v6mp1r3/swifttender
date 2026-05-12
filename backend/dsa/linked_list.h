#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stddef.h>
#include "models/tender.h"

/*
 * ==================================================================
 *  SwiftTender -- Doubly Linked List (Tender Catalogue)
 *  dsa/linked_list.h
 * ==================================================================
 *
 * PURPOSE
 * -------
 * Stores the live tender catalogue as a doubly linked list of
 * TenderNode structs. Each node holds one Tender and two pointers:
 *   prev --> previous node (or NULL if head)
 *   next --> next node     (or NULL if tail)
 *
 * WHY DOUBLY LINKED (not singly)?
 * --------------------------------
 * The key operation that justifies the extra pointer is O(1) removal
 * given a pointer to the node:
 *
 *   Singly linked: to unlink node X you need its PREDECESSOR,
 *   which requires traversing from head -> O(n).
 *
 *   Doubly linked: node X already knows its predecessor via X->prev,
 *   so unlinking is O(1): prev->next = X->next; next->prev = X->prev.
 *
 * Tender cancellation (DELETE /api/tenders/:id) is a frequent
 * operation. O(1) removal is the right trade-off.
 *
 * WHY NOT AN ARRAY?
 * -----------------
 * The number of tenders is unbounded and unknown at compile time.
 * A dynamic array (realloc) would work but would copy the entire
 * array on every resize. A linked list grows in O(1) per insertion
 * with no copying.
 *
 * OPERATIONS & COMPLEXITY
 * -----------------------
 *   ll_prepend    O(1)   -- insert new tender at head (most recent first)
 *   ll_find_id    O(n)   -- linear scan by tender ID
 *   ll_remove     O(1)   -- unlink a node given its pointer
 *   ll_length     O(n)   -- count all nodes
 *   ll_destroy    O(n)   -- free all nodes
 *
 * MEMORY
 * ------
 * Each TenderNode is heap-allocated (malloc). The list owns its nodes.
 * ll_destroy() frees every node and the list struct itself.
 */

/* ── The list container ─────────────────────────────────────────── */
typedef struct {
    TenderNode *head;    /* first node, NULL if empty */
    TenderNode *tail;    /* last  node, NULL if empty */
    size_t      length;  /* number of nodes           */
} LinkedList;

/* ── Lifecycle ──────────────────────────────────────────────────── */
LinkedList *ll_create(void);
void        ll_destroy(LinkedList *list);

/* ── Core operations ────────────────────────────────────────────── */

/*
 * ll_prepend -- insert a tender at the HEAD of the list in O(1).
 *
 * New tenders are prepended so the list is naturally newest-first,
 * which is the default sort order for GET /api/tenders.
 *
 * Returns the new node pointer, or NULL on allocation failure.
 */
TenderNode *ll_prepend(LinkedList *list, const Tender *tender);

/*
 * ll_find_id -- linear search for a tender by its uint32_t id.
 *
 * Returns the node pointer, or NULL if not found.
 * O(n) -- acceptable since n (open tenders) is typically small.
 */
TenderNode *ll_find_id(const LinkedList *list, uint32_t id);

/*
 * ll_remove -- unlink and free a node in O(1).
 *
 * Caller must have a pointer to the node (typically from ll_find_id).
 * The Tender data is gone after this call.
 */
void ll_remove(LinkedList *list, TenderNode *node);

/*
 * ll_length -- return the number of nodes.
 * O(1) -- length is maintained as a field, not recomputed.
 */
size_t ll_length(const LinkedList *list);

/*
 * ll_to_array -- copy all Tender values into a caller-supplied array.
 *
 * Writes at most `max` entries starting at out[0].
 * Returns the number of entries written.
 * Used by GET /api/tenders to serialise the list to JSON.
 */
size_t ll_to_array(const LinkedList *list, Tender *out, size_t max);

#endif /* LINKED_LIST_H */
