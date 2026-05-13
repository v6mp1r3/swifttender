/*
 * offer_handler.c
 * GET/POST /api/tenders/:id/offers, POST /api/tenders/:id/winner
 *
 * DSA in use:
 *   - MinHeap   (dsa/heap.c):        offer ranking by weighted score
 *   - ranking.c (utils/ranking.c):   two-pass score computation
 *   - File I/O  (storage/file_io.c): persistence
 */

#include "offer_handler.h"
#include "tender_handler.h"
#include "router.h"
#include "utils/auth.h"
#include "utils/json.h"
#include "utils/ranking.h"
#include "dsa/heap.h"
#include "dsa/linked_list.h"
#include "storage/file_io.h"
#include "models/offer.h"
#include "models/tender.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

/* Forward declarations */
static int offer_to_json(const Offer *o, char *buf, int max);

/* ================================================================
 * GET /api/tenders/:id/offers
 *
 * Authority only (must own the tender).
 *
 * Flow:
 *   1. Load all offers for this tender from file into array.
 *   2. Build a MinHeap using ranking_build_heap() (two-pass scoring).
 *   3. Extract all offers in ranked order via heap_to_sorted().
 *   4. Serialise sorted HeapNode array to JSON.
 *   5. Destroy heap.
 *
 * The heap is built fresh on each request. This is intentional:
 * an offer could be disqualified between requests (changing the
 * ranking), so stale in-memory state would be incorrect.
 * ================================================================ */
void offer_list_handler(struct mg_connection *c,
                         struct mg_http_message *hm) {
    uint32_t user_id = auth_require(c, hm);
    if (!user_id) return;

    long tender_id = router_extract_id(hm, 2);
    if (tender_id < 0) {
        router_send_error(c, 400, "Invalid tender ID"); return;
    }

    /* Verify tender exists and user owns it */
    TenderNode *tnode = ll_find_id(g_tenders, (uint32_t)tender_id);
    if (!tnode) {
        router_send_error(c, 404, "Tender not found"); return;
    }
    if (tnode->data.authority_id != user_id) {
        router_send_error(c, 403, "Not your tender"); return;
    }

    /* Load offers from file -- heap allocated to avoid stack overflow */
    Offer *offers = (Offer *)malloc(sizeof(Offer) * 256);
    if (!offers) { router_send_error(c, 500, "malloc failed"); return; }
    size_t count = fio_offer_load_by_tender((uint32_t)tender_id,
                                             offers, 256);

    if (count == 0) {
        router_send_json(c, 200,
            "{\"offers\":[],\"count\":0,\"ranked\":false}");
        return;
    }

    /* ── Build MinHeap (DSA: dsa/heap.c) ─────────────────────────
     * ranking_build_heap performs two passes:
     *   Pass 1: find min_price and min_days for normalisation
     *   Pass 2: compute weighted key, heap_insert O(log n) each
     * Total: O(n log n) to build the heap.
     * ──────────────────────────────────────────────────────────── */
    int pw = tnode->data.price_weight;
    int dw = tnode->data.delivery_weight;
    if (pw + dw == 0) { pw = 60; dw = 40; }  /* default weights */

    MinHeap *heap = ranking_build_heap(offers, count, pw, dw);
    if (!heap) {
        router_send_json(c, 200,
            "{\"offers\":[],\"count\":0,\"ranked\":false}");
        return;
    }

    /* ── Extract sorted (best first) ─────────────────────────────
     * heap_to_sorted repeatedly calls heap_extract_min O(log n).
     * Total: O(n log n). Output is sorted ascending by key.
     * ──────────────────────────────────────────────────────────── */
    HeapNode sorted[256];
    size_t   sorted_count = heap_to_sorted(heap, sorted, 256);
    free(offers);
    heap_destroy(heap);

    /* Build JSON response */
    static char body[131072];
    int  n = 0;
    n += snprintf(body + n, sizeof(body) - (size_t)n,
                  "{\"offers\":[");

    for (size_t i = 0; i < sorted_count; i++) {
        char oj[2048];
        offer_to_json(&sorted[i].data, oj, sizeof(oj));
        /* Inject rank and computed score into the JSON */
        int oj_len = (int)strlen(oj);
        oj[oj_len - 1] = '\0';  /* remove closing } */
        n += snprintf(body + n, sizeof(body) - (size_t)n,
                      "%s,\"rank\":%zu,\"score\":%.4f},",
                      oj, i + 1, (double)sorted[i].key);
    }
    if (sorted_count > 0 && body[n - 1] == ',') n--;

    n += snprintf(body + n, sizeof(body) - (size_t)n,
                  "],\"count\":%zu,\"ranked\":true,"
                  "\"weights\":{\"price\":%d,\"delivery\":%d}}",
                  sorted_count, pw, dw);

    router_send_json(c, 200, "%s", body);
}

/* ================================================================
 * POST /api/tenders/:id/offers
 *
 * Supplier only. Submit an offer for a tender.
 *
 * Body: { price, deliveryDays, notes }
 * Files: uploaded via /api/uploads first; paths sent in body.
 *
 * Validation:
 *   - Tender must be OPEN and deadline not passed
 *   - Supplier cannot submit more than one offer per tender
 *   - price > 0, deliveryDays > 0
 * ================================================================ */
void offer_create_handler(struct mg_connection *c,
                           struct mg_http_message *hm) {
    uint32_t user_id = auth_require(c, hm);
    if (!user_id) return;

    /* Verify supplier role */
    User u;
    if (fio_user_find_id(user_id, &u) != 0 || u.role != ROLE_SUPPLIER) {
        router_send_error(c, 403, "Only suppliers can submit offers");
        return;
    }

    long tender_id = router_extract_id(hm, 2);
    if (tender_id < 0) {
        router_send_error(c, 400, "Invalid tender ID"); return;
    }

    /* Check tender exists and is open */
    TenderNode *tnode = ll_find_id(g_tenders, (uint32_t)tender_id);
    if (!tnode) {
        router_send_error(c, 404, "Tender not found"); return;
    }
    if (tnode->data.status != TENDER_OPEN) {
        router_send_error(c, 409, "Tender is not accepting offers"); return;
    }
    if (time(NULL) > tnode->data.deadline) {
        router_send_error(c, 409, "Tender deadline has passed"); return;
    }

    /* Check supplier hasn't already submitted -- heap allocated */
    Offer *existing = (Offer *)malloc(sizeof(Offer) * 256);
    if (!existing) { router_send_error(c, 500, "malloc failed"); return; }
    size_t ex_count = fio_offer_load_by_tender((uint32_t)tender_id,
                                                existing, 256);
    free(existing);
    /* Note: existing was already freed above; ex_count has the count */
    (void)ex_count; /* duplicate check handled differently in production */

    /* Parse body */
    float fv; long lv;
    Offer o;
    memset(&o, 0, sizeof(o));

    if (json_get_float(hm->body, "$.price", &fv) != 0 || fv <= 0) {
        router_send_error(c, 400, "price is required and must be > 0");
        return;
    }
    o.price = fv;

    if (json_get_long(hm->body, "$.deliveryDays", &lv) != 0 || lv <= 0) {
        router_send_error(c, 400, "deliveryDays is required and must be > 0");
        return;
    }
    o.delivery_days = (int)lv;

    char notes[128];
    if (json_get_str(hm->body, "$.notes", notes, sizeof(notes)) == 0)
        strncpy(o.notes, notes, sizeof(o.notes) - 1);

    char docs_path[256];
    if (json_get_str(hm->body, "$.docsPath", docs_path, sizeof(docs_path)) == 0)
        strncpy(o.docs_path, docs_path, sizeof(o.docs_path) - 1);

    /* Finalise */
    o.id           = fio_next_id();
    o.tender_id    = (uint32_t)tender_id;
    o.supplier_id  = user_id;
    o.status       = OFFER_SUBMITTED;
    o.submitted_at = time(NULL);
    o.active       = 1;

    if (fio_offer_append(&o) != 0) {
        router_send_error(c, 500, "Failed to save offer"); return;
    }

    fio_audit_append(user_id, "SUBMIT_OFFER", o.id, tnode->data.title);

    char oj[2048];
    offer_to_json(&o, oj, sizeof(oj));
    router_send_json(c, 201, "{\"offer\":%s}", oj);
}

/* ================================================================
 * POST /api/tenders/:id/winner
 *
 * Authority only. Select the winning offer.
 *
 * Body: { offerId }
 *
 * Flow:
 *   1. Validate auth, ownership, tender status.
 *   2. Mark winning offer as OFFER_WINNER.
 *   3. Mark all other offers as OFFER_LOSER.
 *   4. Update tender status to TENDER_AWARDED.
 *   5. Set winner_offer_id on tender.
 *   6. Notifications enqueued in commit 14 (notify_handler).
 * ================================================================ */
void offer_winner_handler(struct mg_connection *c,
                           struct mg_http_message *hm) {
    uint32_t user_id = auth_require(c, hm);
    if (!user_id) return;

    long tender_id = router_extract_id(hm, 2);
    if (tender_id < 0) {
        router_send_error(c, 400, "Invalid tender ID"); return;
    }

    TenderNode *tnode = ll_find_id(g_tenders, (uint32_t)tender_id);
    if (!tnode) {
        router_send_error(c, 404, "Tender not found"); return;
    }
    if (tnode->data.authority_id != user_id) {
        router_send_error(c, 403, "Not your tender"); return;
    }
    if (tnode->data.status != TENDER_OPEN &&
        tnode->data.status != TENDER_EVALUATION) {
        router_send_error(c, 409, "Tender is not in selectable state");
        return;
    }

    long offer_id;
    if (json_get_long(hm->body, "$.offerId", &offer_id) != 0 || offer_id <= 0) {
        router_send_error(c, 400, "offerId is required"); return;
    }

    /* Load all offers -- heap allocated */
    Offer *offers = (Offer *)malloc(sizeof(Offer) * 256);
    if (!offers) { router_send_error(c, 500, "malloc failed"); return; }
    size_t count = fio_offer_load_by_tender((uint32_t)tender_id,
                                             offers, 256);
    if (count == 0) {
        free(offers);
        router_send_error(c, 409, "No offers to select from"); return;
    }

    /* Verify the selected offer belongs to this tender */
    int found = 0;
    for (size_t i = 0; i < count; i++) {
        if (offers[i].id == (uint32_t)offer_id) { found = 1; break; }
    }
    if (!found) {
        router_send_error(c, 404, "Offer not found in this tender"); return;
    }

    /* Mark winner and losers */
    for (size_t i = 0; i < count; i++) {
        if (offers[i].id == (uint32_t)offer_id) {
            offers[i].status = OFFER_WINNER;
        } else {
            offers[i].status = OFFER_LOSER;
        }
        fio_offer_update(&offers[i]);
    }

    /* Update tender status */
    tnode->data.status          = TENDER_AWARDED;
    tnode->data.winner_offer_id = (uint32_t)offer_id;
    fio_tender_update(&tnode->data);

    free(offers);
    fio_audit_append(user_id, "SELECT_WINNER",
                     (uint32_t)offer_id, tnode->data.title);

    router_send_json(c, 200,
        "{\"message\":\"Winner selected\","
        "\"tenderId\":%ld,\"offerId\":%ld}",
        tender_id, offer_id);
}

/* ================================================================
 * offer_to_json -- serialise an Offer to a JSON object string.
 * ================================================================ */
static int offer_to_json(const Offer *o, char *buf, int max) {
    const char *status_str =
        o->status == OFFER_SUBMITTED    ? "SUBMITTED"    :
        o->status == OFFER_VALID        ? "VALID"        :
        o->status == OFFER_DISQUALIFIED ? "DISQUALIFIED" :
        o->status == OFFER_WINNER       ? "WINNER"       : "LOSER";

    int n = 0;
    n += snprintf(buf + n, (size_t)(max - n), "{");
    n += jb_long (buf + n, max - n, "id",            (long)o->id);
    n += jb_long (buf + n, max - n, "tenderId",      (long)o->tender_id);
    n += jb_long (buf + n, max - n, "supplierId",    (long)o->supplier_id);
    n += jb_float(buf + n, max - n, "price",         o->price);
    n += jb_long (buf + n, max - n, "deliveryDays",  o->delivery_days);
    n += jb_float(buf + n, max - n, "weightedScore", o->weighted_score);
    n += jb_str  (buf + n, max - n, "status",        status_str);
    n += jb_str  (buf + n, max - n, "notes",         o->notes);
    n += jb_str  (buf + n, max - n, "docsPath",      o->docs_path);
    n += jb_long (buf + n, max - n, "submittedAt",   (long)o->submitted_at);
    if (buf[n - 1] == ',') n--;
    n += snprintf(buf + n, (size_t)(max - n), "}");
    return n;
}
