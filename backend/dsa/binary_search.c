/*
 * ==================================================================
 *  SwiftTender -- Binary Search Implementation
 *  dsa/binary_search.c
 * ==================================================================
 */

#include "binary_search.h"

#include <string.h>
#include <stdio.h>

/* ----------------------------------------------------------------
 * bs_find_threshold -- binary search on sorted ThresholdCategory array.
 *
 * Trace example with table = [{GOODS,...}, {SOCIAL,...}, {WORKS,...}]
 * sorted alphabetically, searching for "WORKS":
 *
 *   n=3, low=0, high=2
 *
 *   Iteration 1:
 *     mid = 0 + (2-0)/2 = 1
 *     table[1].code = "SOCIAL"
 *     strcmp("WORKS", "SOCIAL") > 0  → search right half
 *     low = mid+1 = 2
 *
 *   Iteration 2:
 *     mid = 2 + (2-2)/2 = 2
 *     table[2].code = "WORKS"
 *     strcmp("WORKS", "WORKS") == 0  → FOUND, return &table[2]
 *
 * Total: 2 comparisons for n=3.
 * For n=1000: at most log2(1000) ≈ 10 comparisons.
 * ---------------------------------------------------------------- */
const ThresholdCategory *bs_find_threshold(const ThresholdCategory *table,
                                            int n,
                                            const char *code) {
    if (!table || n <= 0 || !code) return NULL;

    int low  = 0;
    int high = n - 1;

    while (low <= high) {
        /* Overflow-safe midpoint: equivalent to (low+high)/2 but
         * avoids undefined behaviour when low+high > INT_MAX      */
        int mid = low + (high - low) / 2;

        int cmp = strncmp(code, table[mid].code, THRESHOLD_CODE_LEN);

        if (cmp == 0) {
            /* Exact match found */
            return &table[mid];

        } else if (cmp < 0) {
            /* code comes BEFORE table[mid] alphabetically
             * → eliminate right half, search left */
            high = mid - 1;

        } else {
            /* code comes AFTER table[mid] alphabetically
             * → eliminate left half, search right */
            low = mid + 1;
        }
    }

    return NULL;   /* not found */
}

/* ----------------------------------------------------------------
 * bs_search_int -- generic binary search on a sorted int array.
 *
 * Demonstrates the general algorithm with the simplest possible
 * comparator (integer less-than).
 *
 * Example: arr = [2, 5, 8, 12, 16, 23, 38, 56, 72, 91], target=23
 *
 *   low=0, high=9
 *
 *   Iter 1: mid=4, arr[4]=16. 23 > 16 → low=5
 *   Iter 2: mid=7, arr[7]=56. 23 < 56 → high=6
 *   Iter 3: mid=5, arr[5]=23. 23 == 23 → return 5
 *
 *   3 comparisons for n=10.  log2(10) ≈ 3.32 ✓
 * ---------------------------------------------------------------- */
int bs_search_int(const int *arr, int n, int target) {
    if (!arr || n <= 0) return -1;

    int low  = 0;
    int high = n - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;

        if (arr[mid] == target) {
            return mid;           /* found: return index */
        } else if (arr[mid] < target) {
            low  = mid + 1;       /* search right half   */
        } else {
            high = mid - 1;       /* search left half    */
        }
    }

    return -1;   /* not found */
}
