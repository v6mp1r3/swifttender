#ifndef TREE_H
#define TREE_H

#include <stddef.h>
#include <stdio.h>

/*
 * ==================================================================
 *  SwiftTender -- N-ary Tree (Quarterly Report Builder)
 *  dsa/tree.h
 * ==================================================================
 *
 * PURPOSE
 * -------
 * Builds a structured quarterly procurement report by modelling the
 * report's natural hierarchy as an N-ary tree, then emitting it via
 * pre-order traversal.
 *
 * The report structure is inherently hierarchical:
 *
 *   Report (root)
 *   ├── Institution
 *   │   ├── Name:  "Liceul Teoretic Nr.1"
 *   │   └── IDNO:  "1007607001234"
 *   ├── Period
 *   │   ├── Quarter: "Q2"
 *   │   └── Year:    "2026"
 *   ├── Tender #1
 *   │   ├── Title:   "Printer paper A4"
 *   │   ├── Value:   "4500.00 MDL"
 *   │   ├── Category:"GOODS"
 *   │   └── Winner
 *   │       ├── Supplier: "SRL PrintPro"
 *   │       ├── Price:    "4200.00 MDL"
 *   │       └── Delivery: "3 days"
 *   ├── Tender #2
 *   │   └── ...
 *   └── Summary
 *       ├── Total procedures: "12"
 *       ├── Total value:      "287450.00 MDL"
 *       └── Unique suppliers: "8"
 *
 * A tree maps this directly. A flat list or array cannot express the
 * nesting without complex indexing or duplication.
 *
 * WHY N-ARY (not binary)?
 * -----------------------
 * A binary tree forces every node to have at most 2 children.
 * Report sections have variable child counts:
 *   - The root may have 20+ tender children (one per procedure)
 *   - Each tender has exactly 4-5 field children
 *   - The summary section has 3 children
 * An N-ary tree with a dynamic children array fits naturally.
 *
 * WHY PRE-ORDER TRAVERSAL?
 * ------------------------
 * Pre-order visits: root → children left-to-right.
 * This matches reading order: a section header appears BEFORE its
 * contents in the output file, exactly as in a printed report.
 *
 *   In-order:  only meaningful for binary trees (left-root-right)
 *   Post-order: parent appears AFTER children -- wrong for reports
 *   Pre-order:  parent appears BEFORE children -- correct ✓
 *
 * NODE TYPES
 * ----------
 * SECTION nodes: have a label and children, no value.
 *   → printed as a header line: "=== Tender #1 ==="
 *
 * FIELD nodes: have a label and a value, no children (leaves).
 *   → printed as a key-value line: "  Title: Printer paper A4"
 *
 * COMPLEXITY
 * ----------
 *   tree_add_child   O(1) amortised  (dynamic child array, doubles)
 *   tree_pre_order   O(n)            (visits every node exactly once)
 *   tree_destroy     O(n)            (post-order free of all nodes)
 *   Space:           O(n)            (one node per label/field)
 */

#define TREE_LABEL_LEN    128
#define TREE_VALUE_LEN    512
#define TREE_INIT_CHILDREN  4   /* initial child array capacity */

typedef enum {
    NODE_SECTION = 0,   /* internal node: has children, no value */
    NODE_FIELD   = 1    /* leaf node: has label + value, no children */
} NodeType;

typedef struct ReportNode {
    NodeType          type;
    char              label[TREE_LABEL_LEN];
    char              value[TREE_VALUE_LEN];  /* only used by NODE_FIELD */
    struct ReportNode **children;             /* dynamic array of child ptrs */
    int               child_count;
    int               child_capacity;
    int               depth;                  /* 0 = root; used for indenting */
} ReportNode;

/* ── Lifecycle ──────────────────────────────────────────────────── */

/* Allocate a SECTION node (internal, will have children). */
ReportNode *tree_section(const char *label, int depth);

/* Allocate a FIELD node (leaf: label + value pair). */
ReportNode *tree_field(const char *label, const char *value, int depth);

/* Add a child to a parent node. Returns 0 on success, -1 on failure. */
int tree_add_child(ReportNode *parent, ReportNode *child);

/* Free the entire tree rooted at node (post-order recursive free). */
void tree_destroy(ReportNode *node);

/* ── Traversal ──────────────────────────────────────────────────── */

/*
 * tree_pre_order -- emit the report to a FILE stream.
 *
 * Visits root first, then children left-to-right recursively.
 * Depth is used to compute indentation (2 spaces per level).
 * SECTION nodes emit a header; FIELD nodes emit "label: value".
 */
void tree_pre_order(const ReportNode *node, FILE *out);

/*
 * tree_pre_order_str -- same as tree_pre_order but writes to a
 * caller-supplied string buffer instead of a file.
 * Used by the API to return the report as a JSON string.
 * Returns number of bytes written.
 */
int tree_pre_order_str(const ReportNode *node, char *buf, int max);

#endif /* TREE_H */
