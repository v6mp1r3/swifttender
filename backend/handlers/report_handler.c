/*
 * report_handler.c
 * GET /api/reports/quarterly, POST /api/uploads
 *
 * DSA in use: N-ary tree (dsa/tree.c)
 *   - Builds report hierarchy as a tree (sections + leaf fields)
 *   - Pre-order traversal emits the structured report text
 *   - tree_pre_order_str() writes into a buffer for JSON response
 */

#include "report_handler.h"
#include "tender_handler.h"
#include "../router.h"
#include "../utils/auth.h"
#include "../utils/json.h"
#include "../dsa/tree.h"
#include "../dsa/linked_list.h"
#include "../storage/file_io.h"
#include "../storage/upload.h"
#include "../models/tender.h"
#include "../models/offer.h"
#include "../models/user.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

/* ================================================================
 * GET /api/reports/quarterly?year=2026&quarter=2
 *
 * Authority only. Generates a quarterly procurement report.
 *
 * Algorithm:
 *   1. Parse year + quarter from query string.
 *   2. Compute start/end timestamps for the quarter.
 *   3. Traverse the tender linked list (O(n)), select tenders
 *      in the quarter that are AWARDED.
 *   4. Build an N-ary tree representing the full report structure.
 *   5. Pre-order traversal serialises tree to a text string.
 *   6. Write report file and return as JSON.
 *
 * N-ary tree structure (see dsa/tree.h for full description):
 *
 *   QUARTERLY REPORT (root)
 *   ├── Institution (section)
 *   │   ├── Name: ...     (field)
 *   │   └── Period: Q2 2026 (field)
 *   ├── Procedure #1 (section) -- one per awarded tender
 *   │   ├── Title: ...    (field)
 *   │   ├── Category: ... (field)
 *   │   ├── Value: ... MDL (field)
 *   │   └── Winner (section)
 *   │       ├── Supplier: ... (field)
 *   │       └── Price: ...    (field)
 *   └── Summary (section)
 *       ├── Total procedures: N  (field)
 *       ├── Total value: ... MDL (field)
 *       └── Unique suppliers: N  (field)
 * ================================================================ */
void report_quarterly_handler(struct mg_connection *c,
                               struct mg_http_message *hm) {
    uint32_t user_id = auth_require(c, hm);
    if (!user_id) return;

    /* Verify authority role */
    User u;
    if (fio_user_find_id(user_id, &u) != 0 || u.role != ROLE_AUTHORITY) {
        router_send_error(c, 403, "Only authorities can generate reports");
        return;
    }

    /* Parse query parameters */
    char year_s[8] = "", quarter_s[4] = "";
    mg_http_get_var(&hm->query, "year",    year_s,    sizeof(year_s));
    mg_http_get_var(&hm->query, "quarter", quarter_s, sizeof(quarter_s));

    int year    = year_s[0]    ? atoi(year_s)    : 2026;
    int quarter = quarter_s[0] ? atoi(quarter_s) : 1;
    if (quarter < 1 || quarter > 4) quarter = 1;

    /* Quarter date ranges */
    int q_start_month = (quarter - 1) * 3 + 1;
    int q_end_month   = q_start_month + 2;

    struct tm t_start = {0}; t_start.tm_year = year - 1900;
    t_start.tm_mon   = q_start_month - 1; t_start.tm_mday = 1;
    struct tm t_end   = {0}; t_end.tm_year  = year - 1900;
    t_end.tm_mon      = q_end_month;      t_end.tm_mday = 1; /* first day of next month */

    time_t ts_start = mktime(&t_start);
    time_t ts_end   = mktime(&t_end);

    /* ── Build N-ary tree (DSA: dsa/tree.c) ───────────────────── */
    char period_label[32];
    snprintf(period_label, sizeof(period_label), "Q%d %d", quarter, year);

    ReportNode *root = tree_section("QUARTERLY PROCUREMENT REPORT", 0);

    /* Institution section */
    ReportNode *inst = tree_section("Institution", 1);
    tree_add_child(inst, tree_field("Name",   u.name,   2));
    tree_add_child(inst, tree_field("IDNO",   u.idno,   2));
    tree_add_child(inst, tree_field("Period", period_label, 2));
    tree_add_child(inst, tree_field("Legal Framework",
                   "HG 870/2022 — Low-Value Public Procurement", 2));
    tree_add_child(root, inst);

    /* Traverse linked list: find awarded tenders in this quarter */
    int    proc_count  = 0;
    float  total_value = 0.0f;
    uint32_t unique_suppliers[256];
    int    unique_count = 0;

    TenderNode *cur = g_tenders ? g_tenders->head : NULL;
    while (cur) {
        Tender *t = &cur->data;

        /* Filter: this authority, this quarter, awarded */
        if (t->authority_id == user_id &&
            t->status == TENDER_AWARDED &&
            t->created_at >= ts_start &&
            t->created_at <  ts_end) {

            proc_count++;
            total_value += t->estimated_value;

            /* Build procedure section node */
            char proc_label[64];
            snprintf(proc_label, sizeof(proc_label),
                     "Procedure #%d", proc_count);
            ReportNode *proc = tree_section(proc_label, 1);

            tree_add_child(proc, tree_field("Title", t->title, 2));

            const char *cat = t->category == CAT_GOODS ? "Goods" :
                              t->category == CAT_WORKS ? "Works" : "Services";
            tree_add_child(proc, tree_field("Category", cat, 2));

            char val[64];
            snprintf(val, sizeof(val), "%.2f MDL", (double)t->estimated_value);
            tree_add_child(proc, tree_field("Estimated Value", val, 2));

            /* Winner subsection */
            Offer winner;
            if (t->winner_offer_id &&
                fio_offer_find_id(t->winner_offer_id, &winner) == 0) {

                ReportNode *win_sec = tree_section("Winner", 2);

                User sup;
                char sup_name[128] = "Unknown";
                if (fio_user_find_id(winner.supplier_id, &sup) == 0)
                    strncpy(sup_name, sup.name, sizeof(sup_name) - 1);

                tree_add_child(win_sec, tree_field("Supplier", sup_name, 3));
                snprintf(val, sizeof(val), "%.2f MDL", (double)winner.price);
                tree_add_child(win_sec, tree_field("Contract Price", val, 3));
                snprintf(val, sizeof(val), "%d days", winner.delivery_days);
                tree_add_child(win_sec, tree_field("Delivery", val, 3));
                tree_add_child(proc, win_sec);

                /* Track unique suppliers */
                int seen = 0;
                for (int i = 0; i < unique_count; i++) {
                    if (unique_suppliers[i] == winner.supplier_id) {
                        seen = 1; break;
                    }
                }
                if (!seen && unique_count < 256)
                    unique_suppliers[unique_count++] = winner.supplier_id;
            }

            tree_add_child(root, proc);
        }
        cur = cur->next;
    }

    /* Summary section */
    ReportNode *summary = tree_section("Summary", 1);
    char val[64];
    snprintf(val, sizeof(val), "%d", proc_count);
    tree_add_child(summary, tree_field("Total Procedures", val, 2));
    snprintf(val, sizeof(val), "%.2f MDL", (double)total_value);
    tree_add_child(summary, tree_field("Total Value", val, 2));
    snprintf(val, sizeof(val), "%d", unique_count);
    tree_add_child(summary, tree_field("Unique Suppliers", val, 2));

    char gen_ts[32];
    time_t now = time(NULL);
    strftime(gen_ts, sizeof(gen_ts), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
    tree_add_child(summary, tree_field("Generated", gen_ts, 2));
    tree_add_child(root, summary);

    /* ── Pre-order traversal -> string buffer ─────────────────── */
    char report_text[32768];
    tree_pre_order_str(root, report_text, sizeof(report_text));
    tree_destroy(root);   /* post-order free */

    /* Write report file */
    struct stat st;
    if (stat("./data/reports", &st) != 0) mkdir("./data/reports", 0755);

    char report_path[256];
    snprintf(report_path, sizeof(report_path),
             "./data/reports/PROCUR_%d_Q%d_%s.txt",
             year, quarter, u.idno);

    FILE *f = fopen(report_path, "w");
    if (f) { fputs(report_text, f); fclose(f); }

    fio_audit_append(user_id, "GENERATE_REPORT", 0, report_path);

    /* Escape report text for JSON embedding */
    char escaped[32768];
    json_escape(report_text, escaped, sizeof(escaped));

    router_send_json(c, 200,
        "{\"period\":\"%s\",\"procedures\":%d,"
        "\"totalValue\":%.2f,\"uniqueSuppliers\":%d,"
        "\"reportPath\":\"%s\",\"report\":\"%s\"}",
        period_label, proc_count, (double)total_value,
        unique_count, report_path, escaped);
}

/* ================================================================
 * POST /api/uploads
 *
 * Generic file upload endpoint. Saves multipart file to
 * data/uploads/{user_id}/ and returns the saved path.
 * Path is then sent with subsequent offer/document creation requests.
 * ================================================================ */
void upload_handler(struct mg_connection *c,
                    struct mg_http_message *hm) {
    uint32_t user_id = auth_require(c, hm);
    if (!user_id) return;

    char saved_path[512] = "";
    if (upload_save(hm, "file", user_id,
                    saved_path, sizeof(saved_path)) != 0) {
        router_send_error(c, 400, "No file found in request"); return;
    }

    fio_audit_append(user_id, "UPLOAD", 0, saved_path);

    router_send_json(c, 200,
        "{\"path\":\"%s\",\"message\":\"File uploaded\"}", saved_path);
}
