#ifndef RANKING_H
#define RANKING_H

#include "models/offer.h"
#include "dsa/heap.h"

/*
 * ranking.h -- weighted score computation and heap-based offer ranking.
 *
 * Scoring formula (lower key = better offer):
 *   price_norm    = offer.price        / min_price_in_set
 *   delivery_norm = offer.delivery_days / min_days_in_set
 *   key = (price_weight/100) * price_norm
 *       + (delivery_weight/100) * delivery_norm
 *
 * The best offer (cheapest AND fastest) gets key = 1.0.
 * All others get key > 1.0. Min-heap root = best offer.
 *
 * ranking_build_heap:
 *   Given an array of Offer structs and the tender's scoring weights,
 *   computes a normalised key for each offer and inserts them into
 *   a MinHeap. Returns the heap ready for extraction.
 *
 * ranking_extract_sorted:
 *   Drains the heap into a sorted HeapNode array (best first).
 *   O(n log n) total -- n heap_extract_min calls each O(log n).
 */

/*
 * ranking_build_heap -- score all offers and insert into a min-heap.
 *
 * Parameters:
 *   offers        : array of Offer structs for one tender
 *   count         : number of offers
 *   price_weight  : integer 0-100 (price percentage)
 *   delivery_weight: integer 0-100 (delivery percentage)
 *
 * Returns a newly allocated MinHeap (caller must heap_destroy() it).
 * Returns NULL on allocation failure.
 */
MinHeap *ranking_build_heap(const Offer *offers, size_t count,
                             int price_weight, int delivery_weight);

/*
 * ranking_score -- compute the weighted key for a single offer
 * given the minimum price and delivery days in the full set.
 * Pure function, no side effects.
 */
float ranking_score(float price, int delivery_days,
                    float min_price, int min_days,
                    int price_weight, int delivery_weight);

#endif /* RANKING_H */
