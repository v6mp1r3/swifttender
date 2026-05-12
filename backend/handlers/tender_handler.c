/*
 * tender_handler.c
 * GET/POST /api/tenders, GET/PATCH/DELETE /api/tenders/:id
 *
 * DSA in use:
 *   - Doubly linked list (dsa/linked_list.c): in-memory tender catalogue
 *   - Binary search    (dsa/binary_search.c): legal threshold lookup
 *   - File I/O layer   (storage/file_io.c):   persistence
 */

#include "tender_handler.h"
#include "../router.h"
#include "../utils/auth.h"
#include "../utils/json.h"
#include "../utils/threshold.h"
#include "../dsa/linked_list.h"
#include "../dsa/binary_search.h"
#include "../storage/file_io.h"
#include "../models/tender.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── Global tender catalogue (doubly linked list) ───────────────── */
/* Declared extern so main.c can initialise it at startup.           */
LinkedList *g_tenders = NULL;

/* ── Global threshold table (loaded from thresholds.csv) ─────────── */
ThresholdCategory g_thresholds[THRESHOLD_MAX_CATEGORIES];
int               g_threshold_count = 0;

/* ── Forward declarations ───────────────────────────────────────── */
static int  tender_to_json(const Tender *t, char *buf, int max);
static void parse_tender_body(struct mg_http_message *hm, Tender *t);

/* ================================================================
 * tender_list_init -- called once from main.c at startup.
 *
 * 1. Load thresholds from CSV (binary search precondition: sorted).
 * 2. Load all active tenders from tenders.bin into the linked list.
 *
 * After this, g_tenders is the live in-memory catalogue.
 * All subsequent requests read/write both the list and the file.
 * ================================================================ */
void tender_list_init(const char *data_dir) {
    /* Load threshold table */
    char csv_path[512];
    snprintf(csv_path, sizeof(csv_path), "%s/thresholds.csv", data_dir);
    g_threshold_count = threshold_load(csv_path, g_thresholds,
                                        THRESHOLD_MAX_CATEGORIES);

    /* Create empty linked list */
    g_tenders = ll_create();
    if (!g_tenders) {
        fprintf(stderr, "[tender] Failed to create tender list\n");
        return;
    }

    /* Load saved tenders from file into the list.
     * We iterate in reverse so prepend produces chronological order
     * (ll_prepend adds at head, so last-loaded ends up first). */
    Tender buf[FIO_MAX_TENDERS];
    size_t n = fio_tender_load_all(buf, FIO_MAX_TENDERS);
    for (int i = (int)n - 1; i >= 0; i--) {
        ll_prepend(g_tenders, &buf[i]);
    }
    printf("[tender] Loaded %zu tenders into linked list\n", n);
}

/* ================================================================
 * GET /api/tenders
 *
 * Optional query params: ?status=OPEN&category=GOODS
 *
 * Traverses the doubly linked list head→tail (newest first) and
 * serialises each active Tender to a JSON array.
 * ================================================================ */
void tender_list_handler(struct mg_connection *c,
                          struct mg_http_message *hm) {
    /* Read optional filter params from query string */
    char status_filter[32]   = "";
    char category_filter[16] = "";
    mg_http_get_var(&hm->query, "status",   status_filter,   sizeof(status_filter));
    mg_http_get_var(&hm->query, "category", category_filter, sizeof(category_filter));

    /* Build JSON array by traversing the linked list */
    char   body[65536];
    int    n   = 0;
    size_t cnt = 0;

    n += snprintf(body + n, sizeof(body) - (size_t)n, "{\"tenders\":[");

    TenderNode *cur = g_tenders ? g_tenders->head : NULL;
    while (cur && n < (int)sizeof(body) - 512) {
        Tender *t = &cur->data;

        /* Apply status filter if provided */
        if (status_filter[0]) {
            const char *s = t->status == TENDER_OPEN       ? "OPEN"       :
                            t->status == TENDER_EVALUATION ? "EVALUATION" :
                            t->status == TENDER_AWARDED    ? "AWARDED"    :
                            t->status == TENDER_CANCELLED  ? "CANCELLED"  : "DRAFT";
            if (strncmp(s, status_filter, sizeof(status_filter)) != 0) {
                cur = cur->next; continue;
            }
        }

        /* Apply category filter */
        if (category_filter[0]) {
            const char *cat = t->category == CAT_GOODS  ? "GOODS"  :
                              t->category == CAT_WORKS  ? "WORKS"  : "SOCIAL";
            if (strncmp(cat, category_filter, sizeof(category_filter)) != 0) {
                cur = cur->next; continue;
            }
        }

        char tj[1024];
        tender_to_json(t, tj, sizeof(tj));
        n += snprintf(body + n, sizeof(body) - (size_t)n, "%s,", tj);
        cnt++;
        cur = cur->next;
    }

    /* Remove trailing comma */
    if (cnt > 0 && body[n - 1] == ',') n--;
    n += snprintf(body + n, sizeof(body) - (size_t)n,
                  "],\"count\":%zu}", cnt);

    router_send_json(c, 200, "%s", body);
}

/* ================================================================
 * POST /api/tenders
 *
 * Authority only. Body: JSON tender fields.
 *
 * Flow:
 *   1. Validate auth + role.
 *   2. Parse body into Tender struct.
 *   3. Binary search threshold check (dsa/binary_search.c).
 *   4. Persist to tenders.bin.
 *   5. Prepend to doubly linked list (O(1)).
 *   6. Return created tender.
 * ================================================================ */
void tender_create_handler(struct mg_connection *c,
                            struct mg_http_message *hm) {
    uint32_t user_id = auth_require(c, hm);
    if (!user_id) return;

    /* Verify user is a contracting authority */
    User u;
    if (fio_user_find_id(user_id, &u) != 0 || u.role != ROLE_AUTHORITY) {
        router_send_error(c, 403, "Only contracting authorities can post tenders");
        return;
    }

    /* Parse request body */
    Tender t;
    memset(&t, 0, sizeof(t));
    parse_tender_body(hm, &t);

    /* Validate required fields */
    if (t.title[0] == '\0' || t.estimated_value <= 0 || t.deadline == 0) {
        router_send_error(c, 400, "Required: title, estimatedValue, deadline, category");
        return;
    }

    /* ── Threshold check using binary search ────────────────────── */
    const char *cat_code = t.category == CAT_GOODS ? "GOODS" :
                           t.category == CAT_WORKS ? "WORKS" : "SOCIAL";
    int tier = threshold_classify(g_thresholds, g_threshold_count,
                                  cat_code, t.estimated_value);

    if (tier == 3) {
        /* Value exceeds low-value limit -- must use MTender */
        router_send_json(c, 422,
            "{\"error\":\"Value exceeds low-value threshold for %s. "
            "Please use MTender (mtender.gov.md) for this procedure.\","
            "\"redirect\":\"https://mtender.gov.md\"}",
            cat_code);
        return;
    }

    /* Finalise the tender struct */
    t.id           = fio_next_id();
    t.authority_id = user_id;
    t.status       = TENDER_OPEN;
    t.created_at   = time(NULL);
    t.active       = 1;

    /* Default weights if not provided */
    if (t.price_weight + t.delivery_weight == 0) {
        t.price_weight    = 60;
        t.delivery_weight = 40;
    }

    /* Persist to file */
    if (fio_tender_append(&t) != 0) {
        router_send_error(c, 500, "Failed to save tender");
        return;
    }

    /* Prepend to doubly linked list -- O(1) */
    ll_prepend(g_tenders, &t);

    fio_audit_append(user_id, "CREATE_TENDER", t.id, t.title);

    char tj[1024];
    tender_to_json(&t, tj, sizeof(tj));
    router_send_json(c, 201, "{\"tender\":%s}", tj);
}

/* ================================================================
 * GET /api/tenders/:id
 *
 * Search the linked list by ID (O(n) linear scan).
 * ================================================================ */
void tender_get_handler(struct mg_connection *c,
                         struct mg_http_message *hm) {
    long id = router_extract_id(hm, 2);  /* segment 2: /api/tenders/<id> */
    if (id < 0) { router_send_error(c, 400, "Invalid tender ID"); return; }

    /* ll_find_id: O(n) traverse -- typical n is small */
    TenderNode *node = ll_find_id(g_tenders, (uint32_t)id);
    if (!node) { router_send_error(c, 404, "Tender not found"); return; }

    char tj[1024];
    tender_to_json(&node->data, tj, sizeof(tj));
    router_send_json(c, 200, "{\"tender\":%s}", tj);
}

/* ================================================================
 * PATCH /api/tenders/:id
 *
 * Authority only. Update title, description, deadline, weights.
 * Not allowed after deadline has passed or status != OPEN.
 * ================================================================ */
void tender_update_handler(struct mg_connection *c,
                            struct mg_http_message *hm) {
    uint32_t user_id = auth_require(c, hm);
    if (!user_id) return;

    long id = router_extract_id(hm, 2);
    if (id < 0) { router_send_error(c, 400, "Invalid tender ID"); return; }

    TenderNode *node = ll_find_id(g_tenders, (uint32_t)id);
    if (!node) { router_send_error(c, 404, "Tender not found"); return; }

    Tender *t = &node->data;

    /* Ownership check */
    if (t->authority_id != user_id) {
        router_send_error(c, 403, "Not your tender"); return;
    }
    if (t->status != TENDER_OPEN) {
        router_send_error(c, 409, "Can only update OPEN tenders"); return;
    }

    /* Apply partial updates from body */
    char tmp[256];
    if (json_get_str(hm->body, "$.title", tmp, sizeof(tmp)) == 0)
        strncpy(t->title, tmp, sizeof(t->title) - 1);
    if (json_get_str(hm->body, "$.description", tmp, sizeof(tmp)) == 0)
        strncpy(t->description, tmp, sizeof(t->description) - 1);

    long lv;
    if (json_get_long(hm->body, "$.deadline", &lv) == 0)
        t->deadline = (time_t)lv;

    float fv;
    if (json_get_float(hm->body, "$.priceWeight", &fv) == 0)
        t->price_weight = (int)fv;
    if (json_get_float(hm->body, "$.deliveryWeight", &fv) == 0)
        t->delivery_weight = (int)fv;

    /* Sync to file */
    fio_tender_update(t);
    fio_audit_append(user_id, "UPDATE_TENDER", t->id, t->title);

    char tj[1024];
    tender_to_json(t, tj, sizeof(tj));
    router_send_json(c, 200, "{\"tender\":%s}", tj);
}

/* ================================================================
 * DELETE /api/tenders/:id
 *
 * Authority only. Soft-delete: sets status=CANCELLED + active=0.
 * Removes from doubly linked list in O(1) (node pointer known).
 * ================================================================ */
void tender_delete_handler(struct mg_connection *c,
                            struct mg_http_message *hm) {
    uint32_t user_id = auth_require(c, hm);
    if (!user_id) return;

    long id = router_extract_id(hm, 2);
    if (id < 0) { router_send_error(c, 400, "Invalid tender ID"); return; }

    TenderNode *node = ll_find_id(g_tenders, (uint32_t)id);
    if (!node) { router_send_error(c, 404, "Tender not found"); return; }

    Tender *t = &node->data;

    if (t->authority_id != user_id) {
        router_send_error(c, 403, "Not your tender"); return;
    }
    if (t->status == TENDER_AWARDED) {
        router_send_error(c, 409, "Cannot cancel an awarded tender"); return;
    }

    /* Soft-delete in file */
    t->status = TENDER_CANCELLED;
    t->active  = 0;
    fio_tender_update(t);

    /* O(1) removal from doubly linked list:
     * node->prev and node->next allow relinking without traversal. */
    ll_remove(g_tenders, node);   /* node is freed here */

    fio_audit_append(user_id, "CANCEL_TENDER", (uint32_t)id, "-");

    router_send_json(c, 200, "{\"message\":\"Tender cancelled\"}");
}

/* ================================================================
 * tender_to_json -- serialise a Tender struct to JSON object string.
 * ================================================================ */
static int tender_to_json(const Tender *t, char *buf, int max) {
    const char *status_str =
        t->status == TENDER_OPEN       ? "OPEN"       :
        t->status == TENDER_EVALUATION ? "EVALUATION" :
        t->status == TENDER_AWARDED    ? "AWARDED"    :
        t->status == TENDER_CANCELLED  ? "CANCELLED"  : "DRAFT";

    const char *cat_str =
        t->category == CAT_GOODS ? "GOODS" :
        t->category == CAT_WORKS ? "WORKS" : "SOCIAL";

    const char *crit_str =
        t->award_criterion == CRIT_LOWEST_PRICE ? "LOWEST_PRICE" :
        t->award_criterion == CRIT_LOWEST_COST  ? "LOWEST_COST"  :
        t->award_criterion == CRIT_BEST_QP      ? "BEST_QP"      : "BEST_QC";

    int n = 0;
    n += snprintf(buf + n, (size_t)(max - n), "{");
    n += jb_long (buf + n, max - n, "id",            (long)t->id);
    n += jb_long (buf + n, max - n, "authorityId",   (long)t->authority_id);
    n += jb_str  (buf + n, max - n, "title",         t->title);
    n += jb_str  (buf + n, max - n, "description",   t->description);
    n += jb_str  (buf + n, max - n, "cpvCode",       t->cpv_code);
    n += jb_str  (buf + n, max - n, "category",      cat_str);
    n += jb_float(buf + n, max - n, "estimatedValue",t->estimated_value);
    n += jb_str  (buf + n, max - n, "awardCriterion",crit_str);
    n += jb_long (buf + n, max - n, "priceWeight",   t->price_weight);
    n += jb_long (buf + n, max - n, "deliveryWeight",t->delivery_weight);
    n += jb_long (buf + n, max - n, "deadline",      (long)t->deadline);
    n += jb_str  (buf + n, max - n, "status",        status_str);
    n += jb_long (buf + n, max - n, "requiredDocs",  t->required_docs);
    n += jb_long (buf + n, max - n, "createdAt",     (long)t->created_at);
    if (buf[n - 1] == ',') n--;
    n += snprintf(buf + n, (size_t)(max - n), "}");
    return n;
}

/* ================================================================
 * parse_tender_body -- extract tender fields from JSON request body.
 * ================================================================ */
static void parse_tender_body(struct mg_http_message *hm, Tender *t) {
    char tmp[256];
    long lv; float fv;

    if (json_get_str(hm->body, "$.title", tmp, sizeof(tmp)) == 0)
        strncpy(t->title, tmp, sizeof(t->title) - 1);
    if (json_get_str(hm->body, "$.description", tmp, sizeof(tmp)) == 0)
        strncpy(t->description, tmp, sizeof(t->description) - 1);
    if (json_get_str(hm->body, "$.cpvCode", tmp, sizeof(tmp)) == 0)
        strncpy(t->cpv_code, tmp, sizeof(t->cpv_code) - 1);

    /* Category */
    if (json_get_str(hm->body, "$.category", tmp, sizeof(tmp)) == 0) {
        if      (strcmp(tmp, "WORKS")  == 0) t->category = CAT_WORKS;
        else if (strcmp(tmp, "SOCIAL") == 0) t->category = CAT_SOCIAL;
        else                                 t->category = CAT_GOODS;
    }

    /* Award criterion */
    if (json_get_str(hm->body, "$.awardCriterion", tmp, sizeof(tmp)) == 0) {
        if      (strcmp(tmp, "LOWEST_COST") == 0) t->award_criterion = CRIT_LOWEST_COST;
        else if (strcmp(tmp, "BEST_QP")     == 0) t->award_criterion = CRIT_BEST_QP;
        else if (strcmp(tmp, "BEST_QC")     == 0) t->award_criterion = CRIT_BEST_QC;
        else                                      t->award_criterion = CRIT_LOWEST_PRICE;
    }

    if (json_get_float(hm->body, "$.estimatedValue",  &fv) == 0) t->estimated_value  = fv;
    if (json_get_long (hm->body, "$.deadline",        &lv) == 0) t->deadline         = (time_t)lv;
    if (json_get_long (hm->body, "$.priceWeight",     &lv) == 0) t->price_weight     = (int)lv;
    if (json_get_long (hm->body, "$.deliveryWeight",  &lv) == 0) t->delivery_weight  = (int)lv;
    if (json_get_long (hm->body, "$.requiredDocs",    &lv) == 0) t->required_docs    = (uint8_t)lv;
}
