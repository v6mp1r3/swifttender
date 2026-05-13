/*
 * ranking.c -- weighted score computation and heap-based offer ranking.
 *
 * DSA in use: MinHeap (dsa/heap.c)
 *   - ranking_build_heap: n * heap_insert calls, each O(log n)
 *   - Total build: O(n log n)
 *   - Extract sorted: O(n log n) via heap_to_sorted
 */

#include "ranking.h"
#include <stdlib.h>
#include <float.h>
#include <limits.h>

/* ----------------------------------------------------------------
 * ranking_score -- compute the weighted key for one offer.
 *
 * Normalisation: divide by the minimum in the set so the best
 * value in each dimension always equals 1.0.
 *
 *   price_norm    = offer_price    / min_price    (1.0 = cheapest)
 *   delivery_norm = offer_days     / min_days     (1.0 = fastest)
 *
 * Weighted combination:
 *   key = (price_weight / 100.0) * price_norm
 *       + (delivery_weight / 100.0) * delivery_norm
 *
 * Example (price_weight=60, delivery_weight=40):
 *   Offer A: price=4200, days=3.  min_price=4200, min_days=1.
 *     price_norm=1.0, delivery_norm=3.0
 *     key = 0.6*1.0 + 0.4*3.0 = 0.6 + 1.2 = 1.80
 *
 *   Offer B: price=4500, days=1.
 *     price_norm=4500/4200=1.071, delivery_norm=1.0
 *     key = 0.6*1.071 + 0.4*1.0 = 0.643 + 0.4 = 1.043
 *
 *   Offer B wins (lower key = better): faster delivery outweighs
 *   slightly higher price under 60/40 weighting.
 * ---------------------------------------------------------------- */
float ranking_score(float price, int delivery_days,
                    float min_price, int min_days,
                    int price_weight, int delivery_weight) {
    /* Guard against division by zero */
    if (min_price <= 0.0f) min_price = price;
    if (min_days  <= 0)    min_days  = delivery_days;

    float pw = (float)price_weight    / 100.0f;
    float dw = (float)delivery_weight / 100.0f;

    float price_norm    = price                / min_price;
    float delivery_norm = (float)delivery_days / (float)min_days;

    return pw * price_norm + dw * delivery_norm;
}

/* ----------------------------------------------------------------
 * ranking_build_heap -- score all offers and insert into a min-heap.
 *
 * Two-pass algorithm:
 *   Pass 1: find min_price and min_days across all valid offers.
 *           These are the normalisation denominators.
 *   Pass 2: compute key for each offer, heap_insert O(log n).
 *
 * Why two passes? Normalisation requires knowing the minimum across
 * the full set before scoring any individual offer.
 * ---------------------------------------------------------------- */
MinHeap *ranking_build_heap(const Offer *offers, size_t count,
                             int price_weight, int delivery_weight) {
    if (!offers || count == 0) return NULL;

    /* Pass 1: find minimums */
    float min_price = FLT_MAX;
    int   min_days  = INT_MAX;

    for (size_t i = 0; i < count; i++) {
        /* Only score SUBMITTED or VALID offers */
        if (offers[i].status == OFFER_DISQUALIFIED) continue;
        if (offers[i].price        < min_price) min_price = offers[i].price;
        if (offers[i].delivery_days < min_days)  min_days  = offers[i].delivery_days;
    }

    /* Nothing scoreable */
    if (min_price == FLT_MAX) return NULL;

    /* Pass 2: score and insert into heap */
    MinHeap *h = heap_create();
    if (!h) return NULL;

    for (size_t i = 0; i < count; i++) {
        if (offers[i].status == OFFER_DISQUALIFIED) continue;

        float key = ranking_score(
            offers[i].price, offers[i].delivery_days,
            min_price, min_days,
            price_weight, delivery_weight
        );

        /* Store computed score back in the offer copy */
        Offer scored = offers[i];
        scored.weighted_score = key;

        heap_insert(h, &scored, key);
    }

    return h;
}
