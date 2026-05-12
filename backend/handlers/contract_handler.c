/*
 * contract_handler.c
 * GET /api/tenders/:id/contract
 * POST /api/tenders/:id/sign
 * POST /api/tenders/:id/documents
 *
 * DSA in use:
 *   - Circular queue (via notify.c): notifications on sign events
 *   - N-ary tree    (dsa/tree.c):   contract document generation
 */

#include "contract_handler.h"
#include "tender_handler.h"
#include "router.h"
#include "utils/auth.h"
#include "utils/json.h"
#include "utils/notify.h"
#include "dsa/tree.h"
#include "dsa/linked_list.h"
#include "storage/file_io.h"
#include "storage/upload.h"
#include "models/tender.h"
#include "models/offer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

/* ── Contract file path helper ─────────────────────────────────── */
static void contract_path(uint32_t tender_id, char *out, size_t max) {
    snprintf(out, max, "./data/contracts/contract_%u.txt", tender_id);
}

/* ── Ensure contracts directory exists ─────────────────────────── */
static void ensure_contracts_dir(void) {
    struct stat st;
    if (stat("./data/contracts", &st) != 0)
        mkdir("./data/contracts", 0755);
}

/* ================================================================
 * generate_contract -- build a contract document using N-ary tree.
 *
 * This is the primary use of the N-ary tree DSA (dsa/tree.c):
 *   1. Build tree: root -> sections -> field leaves
 *   2. Pre-order traversal emits structured text to file
 *
 * Tree structure:
 *   CONTRACT (root, depth=0)
 *   ├── Parties (section, depth=1)
 *   │   ├── Authority: <name> (field, depth=2)
 *   │   └── Supplier:  <name> (field, depth=2)
 *   ├── Subject (section, depth=1)
 *   │   ├── Description: <title> (field, depth=2)
 *   │   └── Value: <price> MDL  (field, depth=2)
 *   ├── Terms (section, depth=1)
 *   │   ├── Delivery: <days> days (field, depth=2)
 *   │   └── Payment:  30 days    (field, depth=2)
 *   └── Signatures (section, depth=1)
 *       ├── Authority: [PENDING / SIGNED] (field, depth=2)
 *       └── Supplier:  [PENDING / SIGNED] (field, depth=2)
 * ================================================================ */
static void generate_contract(const Tender *t, const Offer *o,
                               const char *auth_name,
                               const char *supplier_name,
                               const char *path) {
    /* Build the N-ary tree */
    ReportNode *root = tree_section("SERVICE CONTRACT", 0);
    if (!root) return;

    /* ── Parties section ── */
    ReportNode *parties = tree_section("Parties", 1);
    char val[256];

    snprintf(val, sizeof(val), "%s", auth_name);
    tree_add_child(parties, tree_field("Contracting Authority", val, 2));
    snprintf(val, sizeof(val), "%s", supplier_name);
    tree_add_child(parties, tree_field("Supplier", val, 2));
    tree_add_child(root, parties);

    /* ── Subject section ── */
    ReportNode *subject = tree_section("Subject of Contract", 1);
    tree_add_child(subject, tree_field("Description", t->title, 2));
    snprintf(val, sizeof(val), "%.2f MDL (excl. VAT)", (double)o->price);
    tree_add_child(subject, tree_field("Contract Value", val, 2));
    snprintf(val, sizeof(val), "%s",
             t->category == CAT_GOODS ? "Goods" :
             t->category == CAT_WORKS ? "Works" : "Services");
    tree_add_child(subject, tree_field("Category", val, 2));
    tree_add_child(root, subject);

    /* ── Terms section ── */
    ReportNode *terms = tree_section("Terms", 1);
    snprintf(val, sizeof(val), "%d calendar days", o->delivery_days);
    tree_add_child(terms, tree_field("Delivery Period", val, 2));
    tree_add_child(terms, tree_field("Payment Terms", "30 days after delivery", 2));
    tree_add_child(terms, tree_field("Legal Framework",
                   "Gov. Decision No. 870/2022 (HG 870/2022)", 2));
    tree_add_child(root, terms);

    /* ── Signatures section ── */
    ReportNode *sigs = tree_section("Signatures", 1);
    tree_add_child(sigs, tree_field("Authority Signature", "[PENDING — Sign with MSign]", 2));
    tree_add_child(sigs, tree_field("Supplier Signature",  "[PENDING — Sign with MSign]", 2));

    char ts[32];
    time_t now = time(NULL);
    strftime(ts, sizeof(ts), "%Y-%m-%d", gmtime(&now));
    tree_add_child(sigs, tree_field("Generated On", ts, 2));
    tree_add_child(root, sigs);

    /* Pre-order traversal -> write to file */
    FILE *f = fopen(path, "w");
    if (f) {
        tree_pre_order(root, f);
        fclose(f);
    }

    /* Clean up tree (post-order free) */
    tree_destroy(root);
}

/* ================================================================
 * GET /api/tenders/:id/contract
 *
 * Returns contract status and path. Generates the contract document
 * (via N-ary tree) if it does not exist yet.
 * Accessible by the authority and the winning supplier only.
 * ================================================================ */
void contract_get_handler(struct mg_connection *c,
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

    Tender *t = &tnode->data;
    if (t->status != TENDER_AWARDED) {
        router_send_error(c, 409, "No contract yet — winner not selected");
        return;
    }

    /* Load winning offer */
    Offer winner;
    if (fio_offer_find_id(t->winner_offer_id, &winner) != 0) {
        router_send_error(c, 500, "Winner offer not found"); return;
    }

    /* Verify caller is authority or winning supplier */
    if (user_id != t->authority_id && user_id != winner.supplier_id) {
        router_send_error(c, 403, "Access denied"); return;
    }

    /* Generate contract document if missing */
    ensure_contracts_dir();
    char path[512];
    contract_path((uint32_t)tender_id, path, sizeof(path));

    struct stat st;
    if (stat(path, &st) != 0) {
        /* Document does not exist yet — build it with the N-ary tree */
        User auth_user, sup_user;
        char auth_name[128] = "Contracting Authority";
        char sup_name[128]  = "Supplier";
        if (fio_user_find_id(t->authority_id, &auth_user) == 0)
            strncpy(auth_name, auth_user.name, sizeof(auth_name) - 1);
        if (fio_user_find_id(winner.supplier_id, &sup_user) == 0)
            strncpy(sup_name, sup_user.name, sizeof(sup_name) - 1);

        generate_contract(t, &winner, auth_name, sup_name, path);
        fio_audit_append(user_id, "GENERATE_CONTRACT",
                         (uint32_t)tender_id, path);
    }

    router_send_json(c, 200,
        "{\"tenderId\":%ld,\"contractPath\":\"%s\","
        "\"status\":\"DRAFT\",\"message\":"
        "\"Contract ready. Both parties must sign with MSign.\"}",
        tender_id, path);
}

/* ================================================================
 * POST /api/tenders/:id/sign
 *
 * Mock MSign signing. In production this would verify a qualified
 * electronic signature via Moldova's MSign API.
 *
 * For prototype: marks the contract as signed for the caller's role.
 * When both parties sign, enqueues NOTIF_SIGNED to both users.
 * ================================================================ */
void contract_sign_handler(struct mg_connection *c,
                            struct mg_http_message *hm) {
    uint32_t user_id = auth_require(c, hm);
    if (!user_id) return;

    long tender_id = router_extract_id(hm, 2);
    if (tender_id < 0) {
        router_send_error(c, 400, "Invalid tender ID"); return;
    }

    TenderNode *tnode = ll_find_id(g_tenders, (uint32_t)tender_id);
    if (!tnode || tnode->data.status != TENDER_AWARDED) {
        router_send_error(c, 409, "No awarded contract for this tender");
        return;
    }

    Tender *t = &tnode->data;
    Offer winner;
    if (fio_offer_find_id(t->winner_offer_id, &winner) != 0) {
        router_send_error(c, 500, "Winner offer not found"); return;
    }

    /* Determine signer role */
    int is_authority = (user_id == t->authority_id);
    int is_supplier  = (user_id == winner.supplier_id);

    if (!is_authority && !is_supplier) {
        router_send_error(c, 403, "Not a party to this contract"); return;
    }

    const char *role = is_authority ? "Authority" : "Supplier";
    fio_audit_append(user_id, "SIGN_CONTRACT", (uint32_t)tender_id, role);

    /* Enqueue signing notification to both parties (circular queue) */
    char msg[256];
    snprintf(msg, sizeof(msg),
             "Contract for tender '%s' signed by %s (MSign mock).",
             t->title, role);

    notify_enqueue(t->authority_id,    NOTIF_SIGNED, (uint32_t)tender_id, msg);
    notify_enqueue(winner.supplier_id, NOTIF_SIGNED, (uint32_t)tender_id, msg);

    /* Also enqueue winner/loser notifications if first sign */
    if (is_authority) {
        char winner_msg[256];
        snprintf(winner_msg, sizeof(winner_msg),
                 "You won tender '%s'. Contract is ready to sign.", t->title);
        notify_enqueue(winner.supplier_id, NOTIF_CONTRACT_READY,
                       (uint32_t)tender_id, winner_msg);
    }

    router_send_json(c, 200,
        "{\"message\":\"Contract signed (MSign mock)\","
        "\"signedBy\":\"%s\",\"tenderId\":%ld}", role, tender_id);
}

/* ================================================================
 * POST /api/tenders/:id/documents
 *
 * Supplier uploads final delivery documents (invoice, acceptance act).
 * Enqueues DOC_REQUIRED notification to the authority.
 * ================================================================ */
void contract_docs_handler(struct mg_connection *c,
                            struct mg_http_message *hm) {
    uint32_t user_id = auth_require(c, hm);
    if (!user_id) return;

    long tender_id = router_extract_id(hm, 2);
    if (tender_id < 0) {
        router_send_error(c, 400, "Invalid tender ID"); return;
    }

    TenderNode *tnode = ll_find_id(g_tenders, (uint32_t)tender_id);
    if (!tnode) { router_send_error(c, 404, "Tender not found"); return; }

    /* Save uploaded file */
    char saved_path[512] = "";
    upload_save(hm, "file", user_id, saved_path, sizeof(saved_path));

    fio_audit_append(user_id, "UPLOAD_FINAL_DOCS",
                     (uint32_t)tender_id, saved_path);

    /* Notify authority that documents were uploaded */
    char msg[256];
    snprintf(msg, sizeof(msg),
             "Supplier uploaded final documents for tender '%s'.",
             tnode->data.title);
    notify_enqueue(tnode->data.authority_id, NOTIF_DOC_REQUIRED,
                   (uint32_t)tender_id, msg);

    router_send_json(c, 200,
        "{\"message\":\"Documents uploaded\","
        "\"path\":\"%s\"}", saved_path);
}
