/*
 * ==================================================================
 *  SwiftTender -- Min-Heap Implementation
 *  dsa/heap.c
 * ==================================================================
 */

#include "heap.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ----------------------------------------------------------------
 * Index helpers -- keep parent/child arithmetic in one place.
 *
 * For a 0-indexed array:
 *   parent(i) = (i - 1) / 2   integer division, rounds toward 0
 *   left(i)   = 2*i + 1
 *   right(i)  = 2*i + 2
 *
 * These are the standard formulas for an array-based binary heap.
 * ---------------------------------------------------------------- */
static inline size_t parent(size_t i) { return (i - 1) / 2; }
static inline size_t left  (size_t i) { return 2 * i + 1;   }
static inline size_t right (size_t i) { return 2 * i + 2;   }

/* ----------------------------------------------------------------
 * swap -- exchange two HeapNode elements.
 * Called during sift-up and sift-down to restore heap property.
 * ---------------------------------------------------------------- */
static inline void swap(HeapNode *a, HeapNode *b) {
    HeapNode tmp = *a;
    *a = *b;
    *b = tmp;
}

/* ----------------------------------------------------------------
 * heap_create -- allocate an empty min-heap.
 * ---------------------------------------------------------------- */
MinHeap *heap_create(void) {
    MinHeap *h = malloc(sizeof(MinHeap));
    if (!h) return NULL;

    h->data = malloc(HEAP_INITIAL_CAP * sizeof(HeapNode));
    if (!h->data) { free(h); return NULL; }

    h->size     = 0;
    h->capacity = HEAP_INITIAL_CAP;
    return h;
}

/* ----------------------------------------------------------------
 * heap_destroy -- free all heap memory.
 * ---------------------------------------------------------------- */
void heap_destroy(MinHeap *h) {
    if (!h) return;
    free(h->data);
    free(h);
}

/* ----------------------------------------------------------------
 * sift_up -- restore heap property after inserting at the end.
 *
 * After appending the new element at index `i = size - 1`, we
 * compare it with its parent. If the new element's key is smaller
 * than its parent's (violating the min-heap property), we swap them
 * and continue upward until we reach the root or find a parent
 * with a smaller or equal key.
 *
 * Example with keys [1, 3, 5, 7, 9] and inserting key=2:
 *
 *   Array before:  1  3  5  7  9  [2]  <- new element at index 5
 *                  0  1  2  3  4   5
 *
 *   Step 1: i=5, parent=2 (key=5). 2 < 5 → swap
 *   Array:  1  3  2  7  9  5
 *
 *   Step 2: i=2, parent=0 (key=1). 2 > 1 → stop
 *   Final:  1  3  2  7  9  5   <- valid min-heap
 * ---------------------------------------------------------------- */
static void sift_up(MinHeap *h, size_t i) {
    while (i > 0 && h->data[i].key < h->data[parent(i)].key) {
        swap(&h->data[i], &h->data[parent(i)]);
        i = parent(i);
    }
}

/* ----------------------------------------------------------------
 * sift_down -- restore heap property after removing the root.
 *
 * After moving the last element to index 0 (the root), we compare
 * it with its smaller child. If it is larger than the smaller child,
 * we swap and continue downward until both children are larger or
 * we reach a leaf.
 *
 * Example: root removed, last element (key=9) moved to root:
 *
 *   Array:  9  3  2  7  5   (key=9 at root)
 *
 *   Step 1: i=0. left=1(3), right=2(2). smaller child=2 at idx 2.
 *           9 > 2 → swap(0, 2)
 *   Array:  2  3  9  7  5
 *
 *   Step 2: i=2. left=5 (out of bounds) → stop (leaf node)
 *   Final:  2  3  9  7  5   <- valid min-heap, root=2
 * ---------------------------------------------------------------- */
static void sift_down(MinHeap *h, size_t i) {
    size_t smallest = i;
    size_t l = left(i);
    size_t r = right(i);

    /* Find the smallest among node and its children */
    if (l < h->size && h->data[l].key < h->data[smallest].key)
        smallest = l;
    if (r < h->size && h->data[r].key < h->data[smallest].key)
        smallest = r;

    if (smallest != i) {
        swap(&h->data[i], &h->data[smallest]);
        sift_down(h, smallest);   /* recurse down the affected subtree */
    }
}

/* ----------------------------------------------------------------
 * heap_insert -- add a new offer with its ranking key.
 *
 * 1. Grow the array if full (double capacity).
 * 2. Append new element at the end.
 * 3. Sift up to restore heap property.
 *
 * Amortised O(log n): doubling keeps total copy work O(n).
 * ---------------------------------------------------------------- */
int heap_insert(MinHeap *h, const Offer *offer, float key) {
    if (!h || !offer) return -1;

    /* Grow if needed */
    if (h->size == h->capacity) {
        size_t   new_cap  = h->capacity * 2;
        HeapNode *new_data = realloc(h->data, new_cap * sizeof(HeapNode));
        if (!new_data) return -1;
        h->data     = new_data;
        h->capacity = new_cap;
    }

    /* Append at the end */
    size_t i       = h->size;
    h->data[i].data = *offer;   /* copy full Offer struct */
    h->data[i].key  = key;
    h->size++;

    /* Restore heap property */
    sift_up(h, i);
    return 0;
}

/* ----------------------------------------------------------------
 * heap_peek -- return the root (best offer) without removing it.
 * O(1) -- root is always at index 0.
 * ---------------------------------------------------------------- */
HeapNode heap_peek(const MinHeap *h) {
    /* Return a zeroed node if heap is empty */
    if (!h || h->size == 0) {
        HeapNode empty;
        memset(&empty, 0, sizeof(empty));
        return empty;
    }
    return h->data[0];
}

/* ----------------------------------------------------------------
 * heap_extract_min -- remove and return the root (best offer).
 *
 * 1. Save the root (best offer).
 * 2. Move last element to root position.
 * 3. Shrink size.
 * 4. Sift down from root to restore heap property.
 *
 * This is O(log n): sift_down visits at most log2(n) levels.
 * ---------------------------------------------------------------- */
HeapNode heap_extract_min(MinHeap *h) {
    HeapNode result;
    memset(&result, 0, sizeof(result));

    if (!h || h->size == 0) return result;

    result = h->data[0];               /* save the minimum (root)    */
    h->data[0] = h->data[h->size - 1]; /* move last element to root  */
    h->size--;

    if (h->size > 0) sift_down(h, 0); /* restore heap property       */

    return result;
}

/* ----------------------------------------------------------------
 * heap_size / heap_is_empty -- simple accessors.
 * ---------------------------------------------------------------- */
size_t heap_size(const MinHeap *h) {
    return h ? h->size : 0;
}

int heap_is_empty(const MinHeap *h) {
    return (!h || h->size == 0) ? 1 : 0;
}

/* ----------------------------------------------------------------
 * heap_to_sorted -- extract all nodes into a sorted array.
 *
 * Repeatedly calls heap_extract_min() so the output array is
 * in ascending key order (best offer first).
 *
 * WARNING: this empties the heap. The caller should work on a copy
 * if the original heap needs to be preserved.
 *
 * Overall complexity: O(n log n) -- n extractions each O(log n).
 * ---------------------------------------------------------------- */
size_t heap_to_sorted(MinHeap *h, HeapNode *out, size_t max) {
    if (!h || !out || max == 0) return 0;

    size_t written = 0;
    while (!heap_is_empty(h) && written < max) {
        out[written++] = heap_extract_min(h);
    }
    return written;
}
