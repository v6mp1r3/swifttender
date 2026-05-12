#ifndef TENDER_H
#define TENDER_H

#include <stdint.h>
#include <time.h>

/* ── Procurement category ─────────────────────────────────── */
typedef enum {
    CAT_GOODS   = 0,
    CAT_WORKS   = 1,
    CAT_SOCIAL  = 2
} ProcCategory;

/* ── Award criterion (HG 870/2022 pt.25) ─────────────────── */
typedef enum {
    CRIT_LOWEST_PRICE  = 0,
    CRIT_LOWEST_COST   = 1,
    CRIT_BEST_QP       = 2,   /* best quality-price ratio */
    CRIT_BEST_QC       = 3    /* best quality-cost ratio  */
} AwardCriterion;

/* ── Tender lifecycle status ──────────────────────────────── */
typedef enum {
    TENDER_DRAFT      = 0,
    TENDER_OPEN       = 1,
    TENDER_EVALUATION = 2,
    TENDER_AWARDED    = 3,
    TENDER_CANCELLED  = 4
} TenderStatus;

/*
 * Required documents bitmask — each bit = one document type.
 * Contracting authority sets which docs suppliers must upload.
 *
 * Bit 0: company registration certificate
 * Bit 1: VAT certificate
 * Bit 2: professional license
 * Bit 3: quality certificate (ISO etc.)
 * Bit 4: environmental certificate
 * Bit 5: conflict of interest declaration
 * Bit 6: technical specification compliance doc
 * Bit 7: reserved
 */
#define DOC_REGISTRATION (1 << 0)
#define DOC_VAT_CERT     (1 << 1)
#define DOC_PROF_LIC     (1 << 2)
#define DOC_QUALITY_CERT (1 << 3)
#define DOC_ENV_CERT     (1 << 4)
#define DOC_CONFLICT_DECL (1 << 5)
#define DOC_TECH_SPEC    (1 << 6)

/* ── Tender struct (node in doubly linked list) ───────────── */
typedef struct {
    uint32_t      id;
    uint32_t      authority_id;       /* FK → User */
    char          title[256];
    char          description[1024];
    char          cpv_code[16];       /* optional CPV classification code */
    ProcCategory  category;
    float         estimated_value;    /* MDL excl. VAT */
    AwardCriterion award_criterion;
    int           price_weight;       /* 0–100, rest goes to quality/delivery */
    int           delivery_weight;    /* 0–100 */
    time_t        deadline;           /* offer submission deadline */
    TenderStatus  status;
    uint8_t       required_docs;      /* bitmask of required supplier docs */
    char          description_path[256]; /* optional attached PDF */
    uint32_t      winner_offer_id;    /* set after award */
    time_t        created_at;
    int           active;             /* soft-delete flag */
} Tender;

/* ── Doubly linked list node ──────────────────────────────── */
typedef struct TenderNode {
    Tender            data;
    struct TenderNode *prev;
    struct TenderNode *next;
} TenderNode;

#endif /* TENDER_H */
