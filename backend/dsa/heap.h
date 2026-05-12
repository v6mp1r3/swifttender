#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include "models/offer.h"

/*
 * ==================================================================
 *  SwiftTender -- Min-Heap (Offer Ranking)
 *  dsa/heap.h
 * ==================================================================
 *
 * PURPOSE
 * -------
 * Ranks supplier offers for a tender by weighted score.
 * The offer with the LOWEST score sits at the root — meaning it is
 * the economically most advantageous offer (best price + delivery).
 *
 * SCORING CONVENTION (lower = better)
 * ------------------------------------
 * Each offer's key is computed in ranking.c as:
 *
 *   price_norm    = offer.price        / min_price_in_set
 *   delivery_norm = offer.delivery_days / min_days_in_set
 *
 *   key = price_weight    * price_norm
 *       + delivery_weight * delivery_norm
 *
 * The cheapest + fastest offer gets key = 1.0 (ideal).
 * All others get key > 1.0 (worse). Min-heap root = best offer.
 *
 * WHY A HEAP (not insertion sort like the CLI prototype)?
 * -------------------------------------------------------
 * The CLI tool always had exactly 3 quotes -- insertion sort was
 * trivially correct. A web platform can receive N offers from any
 * number of registered suppliers. Heap gives:
 *
 *   insert     O(log n)   -- each offer submitted via API
 *   peek-min   O(1)       -- view the current best offer instantly
 *   extract    O(log n)   -- pop offers in ranked order for display
 *
 * vs. keeping a sorted array: O(n) insert each time.
 *
 * ARRAY REPRESENTATION
 * --------------------
 * A binary heap maps to a flat array with no wasted space:
 *
 *   parent of node i  : (i - 1) / 2
 *   left  child of i  : 2*i + 1
 *   right child of i  : 2*i + 2
 *
 * The heap property: key[parent] <= key[child] for all nodes.
 *
 * OPERATIONS
 * ----------
 *   heap_insert      O(log n)  -- add offer, sift up
 *   heap_peek        O(1)      -- view root (best offer) without removing
 *   heap_extract_min O(log n)  -- remove root, sift down
 *   heap_size        O(1)
 *   heap_to_sorted   O(n log n)-- copy heap, extract all in order
 */

#define HEAP_INITIAL_CAP 16   /* grows dynamically by doubling */

typedef struct {
    HeapNode *data;     /* array of HeapNode (defined in models/offer.h) */
    size_t    size;     /* current number of elements                    */
    size_t    capacity; /* allocated slots                               */
} MinHeap;

/* Lifecycle */
MinHeap *heap_create(void);
void     heap_destroy(MinHeap *h);

/* Core operations */
int      heap_insert     (MinHeap *h, const Offer *offer, float key);
HeapNode heap_peek       (const MinHeap *h);
HeapNode heap_extract_min(MinHeap *h);

/* Utility */
size_t   heap_size       (const MinHeap *h);
int      heap_is_empty   (const MinHeap *h);

/*
 * heap_to_sorted -- extract all offers into a caller-supplied array,
 * sorted best-first (lowest key first).
 *
 * NOTE: this EMPTIES the heap. Call on a copy if you need to keep it.
 * Returns number of elements written.
 */
size_t heap_to_sorted(MinHeap *h, HeapNode *out, size_t max);

#endif /* HEAP_H */
