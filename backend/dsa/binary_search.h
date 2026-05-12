#ifndef BINARY_SEARCH_H
#define BINARY_SEARCH_H

#include <stddef.h>
#include "utils/threshold.h"

/*
 * ==================================================================
 *  SwiftTender -- Binary Search (Legal Threshold Lookup)
 *  dsa/binary_search.h
 * ==================================================================
 *
 * PURPOSE
 * -------
 * Finds the legal procurement threshold for a given category code
 * in a sorted ThresholdCategory array loaded from thresholds.csv.
 *
 * Called on every POST /api/tenders to enforce Moldova's legal
 * limits (HG 870/2022): if estimated_value exceeds the threshold
 * for the tender's category, the request is rejected with a
 * redirect-to-MTender message.
 *
 * WHY BINARY SEARCH?
 * ------------------
 * The threshold table is small (~3 categories), static, and sorted
 * by category code at load time. These are the exact conditions
 * where binary search is appropriate:
 *
 *   1. SORTED data    → binary search requires sorted input
 *   2. STATIC data    → sort cost paid once at startup
 *   3. POINT QUERY    → we want one exact match, not a range
 *   4. REPEATED calls → O(log n) per call vs O(n) linear scan
 *
 * For n=3 the difference is trivial at runtime, but the algorithm
 * is correct for any n and demonstrates the principle cleanly.
 *
 * THE ALGORITHM
 * -------------
 * Divide and conquer on a sorted array:
 *
 *   low = 0, high = n-1
 *   while low <= high:
 *     mid = low + (high - low) / 2   <- avoids integer overflow
 *     if key == array[mid].code  → found, return &array[mid]
 *     if key  < array[mid].code  → search left half  (high = mid-1)
 *     if key  > array[mid].code  → search right half (low  = mid+1)
 *   return NULL   (not found)
 *
 * Each iteration eliminates half the remaining search space.
 *
 * COMPLEXITY
 * ----------
 *   Time:  O(log n) -- halves the search space each step
 *   Space: O(1)     -- three index variables, no extra allocation
 *
 * vs. linear scan: O(n) time -- worse for large n, same for n=3
 *
 * OVERFLOW-SAFE MIDPOINT
 * ----------------------
 * We use mid = low + (high - low) / 2 instead of (low + high) / 2.
 * The naive formula overflows when low + high > INT_MAX.
 * The safe formula is mathematically identical but never overflows.
 */

/*
 * bs_find_threshold -- binary search for a ThresholdCategory by code.
 *
 * Parameters:
 *   table  : sorted array of ThresholdCategory (sorted by .code)
 *   n      : number of entries in the array
 *   code   : category code string to search for (e.g. "GOODS")
 *
 * Returns pointer to the matching entry, or NULL if not found.
 *
 * PRECONDITION: table must be sorted ascending by .code (strcmp order).
 */
const ThresholdCategory *bs_find_threshold(const ThresholdCategory *table,
                                            int n,
                                            const char *code);

/*
 * bs_search_int -- generic binary search on a sorted int array.
 *
 * Included to demonstrate the general algorithm independent of the
 * domain-specific threshold use case.
 *
 * Returns the index of `target` in `arr`, or -1 if not found.
 * PRECONDITION: arr must be sorted ascending.
 */
int bs_search_int(const int *arr, int n, int target);

#endif /* BINARY_SEARCH_H */
