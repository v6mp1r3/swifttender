#ifndef OFFER_H
#define OFFER_H

#include <stdint.h>
#include <time.h>

/* ── Offer lifecycle status ───────────────────────────────── */
typedef enum {
    OFFER_SUBMITTED    = 0,
    OFFER_VALID        = 1,
    OFFER_DISQUALIFIED = 2,
    OFFER_WINNER       = 3,
    OFFER_LOSER        = 4
} OfferStatus;

/* ── Offer struct (stored in min-heap per tender) ─────────── */
typedef struct {
    uint32_t     id;
    uint32_t     tender_id;       /* FK -> Tender */
    uint32_t     supplier_id;     /* FK -> User   */
    float        price;           /* offered price, MDL excl. VAT */
    int          delivery_days;   /* proposed delivery time in days */
    float        quality_score;   /* 0.0-100.0, set by authority if applicable */
    float        weighted_score;  /* computed: price_w*price_norm + del_w*del_norm */
    OfferStatus  status;
    time_t       submitted_at;
    char         docs_path[256];  /* folder holding all uploaded docs for this offer */
    char         notes[128];      /* optional supplier notes */
    int          active;
} Offer;

/* ── Heap node (min-heap ordered by weighted_score ascending) */
typedef struct {
    Offer    data;
    float    key;   /* = weighted_score; lower is better */
} HeapNode;

#endif /* OFFER_H */
