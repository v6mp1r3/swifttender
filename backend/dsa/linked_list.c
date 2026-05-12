/*
 * ==================================================================
 *  SwiftTender -- Doubly Linked List Implementation
 *  dsa/linked_list.c
 * ==================================================================
 */

#include "linked_list.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ----------------------------------------------------------------
 * ll_create -- allocate and initialise an empty list.
 * ---------------------------------------------------------------- */
LinkedList *ll_create(void) {
    LinkedList *list = malloc(sizeof(LinkedList));
    if (!list) return NULL;
    list->head   = NULL;
    list->tail   = NULL;
    list->length = 0;
    return list;
}

/* ----------------------------------------------------------------
 * ll_destroy -- free every node and the list struct itself.
 *
 * Walks from head to tail, freeing each node before moving on.
 * We must save next BEFORE freeing the current node, otherwise
 * we would read freed memory.
 * ---------------------------------------------------------------- */
void ll_destroy(LinkedList *list) {
    if (!list) return;

    TenderNode *cur = list->head;
    while (cur) {
        TenderNode *next = cur->next;   /* save before free */
        free(cur);
        cur = next;
    }
    free(list);
}

/* ----------------------------------------------------------------
 * ll_prepend -- insert a new node at the HEAD in O(1).
 *
 * Steps (with a non-empty list):
 *
 *   Before:   [head] <-> [A] <-> [B] <-> [tail=B]
 *
 *   1. Allocate new node N, copy Tender data into it.
 *   2. N->next = old head.
 *   3. N->prev = NULL  (N is the new head).
 *   4. old_head->prev = N.
 *   5. list->head = N.
 *
 *   After:    [head=N] <-> [A] <-> [B] <-> [tail=B]
 *
 * Empty list case: both head and tail point to the new node.
 * ---------------------------------------------------------------- */
TenderNode *ll_prepend(LinkedList *list, const Tender *tender) {
    if (!list || !tender) return NULL;

    TenderNode *node = malloc(sizeof(TenderNode));
    if (!node) return NULL;

    /* Copy tender data into the node (node owns a full struct copy) */
    memcpy(&node->data, tender, sizeof(Tender));
    node->prev = NULL;
    node->next = list->head;

    if (list->head) {
        /* Link old head's prev back to the new node */
        list->head->prev = node;
    } else {
        /* List was empty: new node is also the tail */
        list->tail = node;
    }

    list->head = node;
    list->length++;
    return node;
}

/* ----------------------------------------------------------------
 * ll_find_id -- linear scan for a tender by ID.
 *
 * Starts at head and walks ->next until the ID matches or the
 * list ends. O(n) is acceptable: the number of open low-value
 * tenders at any one institution is small (tens, not millions).
 * ---------------------------------------------------------------- */
TenderNode *ll_find_id(const LinkedList *list, uint32_t id) {
    if (!list) return NULL;

    TenderNode *cur = list->head;
    while (cur) {
        if (cur->data.id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

/* ----------------------------------------------------------------
 * ll_remove -- unlink and free a node in O(1).
 *
 * Four pointer updates cover all cases:
 *
 *   Case 1: node has both prev and next (middle of list)
 *     prev->next = node->next
 *     next->prev = node->prev
 *
 *   Case 2: node is the HEAD (no prev)
 *     list->head = node->next
 *     if new head exists: new_head->prev = NULL
 *
 *   Case 3: node is the TAIL (no next)
 *     list->tail = node->prev
 *     if new tail exists: new_tail->next = NULL
 *
 *   Case 4: node is both head and tail (single-element list)
 *     list->head = list->tail = NULL
 * ---------------------------------------------------------------- */
void ll_remove(LinkedList *list, TenderNode *node) {
    if (!list || !node) return;

    /* Stitch previous node's next pointer */
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        /* node was the head */
        list->head = node->next;
    }

    /* Stitch next node's prev pointer */
    if (node->next) {
        node->next->prev = node->prev;
    } else {
        /* node was the tail */
        list->tail = node->prev;
    }

    list->length--;
    free(node);
}

/* ----------------------------------------------------------------
 * ll_length -- O(1) length query (maintained as a field).
 * ---------------------------------------------------------------- */
size_t ll_length(const LinkedList *list) {
    return list ? list->length : 0;
}

/* ----------------------------------------------------------------
 * ll_to_array -- copy Tender values into a flat array for JSON output.
 *
 * Walks the list from head (newest) to tail (oldest) and copies
 * each Tender struct into the caller-supplied output array.
 * Stops at `max` entries to avoid buffer overflow.
 *
 * Returns the number of entries actually written.
 * ---------------------------------------------------------------- */
size_t ll_to_array(const LinkedList *list, Tender *out, size_t max) {
    if (!list || !out || max == 0) return 0;

    size_t     written = 0;
    TenderNode *cur    = list->head;

    while (cur && written < max) {
        memcpy(&out[written], &cur->data, sizeof(Tender));
        written++;
        cur = cur->next;
    }
    return written;
}
