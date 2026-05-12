/*
 * ==================================================================
 *  SwiftTender -- N-ary Tree Implementation
 *  dsa/tree.c
 * ==================================================================
 */

#include "tree.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ----------------------------------------------------------------
 * alloc_node -- internal helper: allocate and zero a ReportNode.
 * ---------------------------------------------------------------- */
static ReportNode *alloc_node(NodeType type, const char *label, int depth) {
    ReportNode *node = calloc(1, sizeof(ReportNode));
    if (!node) return NULL;

    node->type           = type;
    node->depth          = depth;
    node->child_count    = 0;
    node->child_capacity = 0;
    node->children       = NULL;

    strncpy(node->label, label, TREE_LABEL_LEN - 1);
    node->label[TREE_LABEL_LEN - 1] = '\0';

    return node;
}

/* ----------------------------------------------------------------
 * tree_section -- allocate an internal (section) node.
 *
 * Section nodes group related fields. Examples:
 *   tree_section("Institution", 1)
 *   tree_section("Tender #3", 1)
 *   tree_section("Winner", 2)
 * ---------------------------------------------------------------- */
ReportNode *tree_section(const char *label, int depth) {
    return alloc_node(NODE_SECTION, label, depth);
}

/* ----------------------------------------------------------------
 * tree_field -- allocate a leaf (field) node with a value.
 *
 * Field nodes are the leaves of the tree -- they hold actual data.
 * Examples:
 *   tree_field("Title",    "Printer paper A4", 2)
 *   tree_field("Price",    "4200.00 MDL",      3)
 *   tree_field("Delivery", "3 days",           3)
 * ---------------------------------------------------------------- */
ReportNode *tree_field(const char *label, const char *value, int depth) {
    ReportNode *node = alloc_node(NODE_FIELD, label, depth);
    if (!node) return NULL;

    strncpy(node->value, value, TREE_VALUE_LEN - 1);
    node->value[TREE_VALUE_LEN - 1] = '\0';

    return node;
}

/* ----------------------------------------------------------------
 * tree_add_child -- append a child pointer to a parent node.
 *
 * Uses a dynamic array with doubling strategy:
 *   - Starts at capacity TREE_INIT_CHILDREN (4).
 *   - When full, reallocates at 2× the current capacity.
 *
 * This keeps amortised insertion O(1) while avoiding a fixed limit
 * on how many children a section can have (e.g. the root section
 * may have dozens of tender children).
 *
 * The children array stores POINTERS (ReportNode*), not copies.
 * The parent does not own the child data here -- tree_destroy()
 * must free the whole tree recursively.
 * ---------------------------------------------------------------- */
int tree_add_child(ReportNode *parent, ReportNode *child) {
    if (!parent || !child) return -1;

    /* Grow child pointer array if needed */
    if (parent->child_count == parent->child_capacity) {
        int new_cap = (parent->child_capacity == 0)
                      ? TREE_INIT_CHILDREN
                      : parent->child_capacity * 2;

        ReportNode **new_arr = realloc(parent->children,
                                       (size_t)new_cap * sizeof(ReportNode *));
        if (!new_arr) return -1;

        parent->children       = new_arr;
        parent->child_capacity = new_cap;
    }

    parent->children[parent->child_count++] = child;
    return 0;
}

/* ----------------------------------------------------------------
 * tree_destroy -- recursively free the entire tree (post-order).
 *
 * Post-order: free children before freeing the parent.
 * This is correct because the parent's children array holds pointers
 * to heap-allocated nodes -- we must free children first, then free
 * the parent's children array, then free the parent itself.
 *
 * Pre-order free would be WRONG: freeing the parent first would
 * lose the children array pointer before we could walk it.
 *
 * Recursion depth = tree depth (typically 3-4 levels for a report).
 * ---------------------------------------------------------------- */
void tree_destroy(ReportNode *node) {
    if (!node) return;

    /* Recursively free all children first (post-order) */
    for (int i = 0; i < node->child_count; i++) {
        tree_destroy(node->children[i]);
    }

    free(node->children);   /* free the pointer array itself */
    free(node);             /* free the node struct          */
}

/* ----------------------------------------------------------------
 * indent -- write `depth * 2` spaces to a FILE for readability.
 * ---------------------------------------------------------------- */
static void indent(FILE *out, int depth) {
    for (int i = 0; i < depth * 2; i++) fputc(' ', out);
}

/* ----------------------------------------------------------------
 * tree_pre_order -- emit the report tree to a FILE stream.
 *
 * Visit order: root → children[0] → children[1] → ... (recursive)
 *
 * Output format:
 *
 *   === Institution ===          <- SECTION node (depth=1)
 *     Name: Liceul Teoretic Nr.1 <- FIELD node   (depth=2)
 *     IDNO: 1007607001234        <- FIELD node   (depth=2)
 *   === Tender #1 ===            <- SECTION node (depth=1)
 *     Title: Printer paper A4    <- FIELD node   (depth=2)
 *     === Winner ===             <- SECTION node (depth=2)
 *       Supplier: SRL PrintPro  <- FIELD node   (depth=3)
 *
 * The recursive call naturally handles arbitrary nesting depth.
 * ---------------------------------------------------------------- */
void tree_pre_order(const ReportNode *node, FILE *out) {
    if (!node || !out) return;

    if (node->type == NODE_SECTION) {
        /* Section header: indented, wrapped in === === */
        indent(out, node->depth);
        fprintf(out, "=== %s ===\n", node->label);
    } else {
        /* Field leaf: indented key: value pair */
        indent(out, node->depth);
        fprintf(out, "%s: %s\n", node->label, node->value);
    }

    /* Pre-order: visit children AFTER the parent */
    for (int i = 0; i < node->child_count; i++) {
        tree_pre_order(node->children[i], out);
    }
}

/* ----------------------------------------------------------------
 * tree_pre_order_str -- same traversal but writes into a string.
 *
 * Uses snprintf with a running offset to build the output
 * incrementally without exceeding the buffer.
 *
 * Returns total bytes written (excluding null terminator).
 * ---------------------------------------------------------------- */
static int pre_order_into(const ReportNode *node,
                           char *buf, int max, int offset) {
    if (!node || offset >= max) return offset;

    int written = 0;
    int remaining = max - offset;

    if (node->type == NODE_SECTION) {
        written = snprintf(buf + offset, (size_t)remaining,
                           "%*s=== %s ===\n",
                           node->depth * 2, "", node->label);
    } else {
        written = snprintf(buf + offset, (size_t)remaining,
                           "%*s%s: %s\n",
                           node->depth * 2, "", node->label, node->value);
    }

    if (written > 0) offset += written;

    for (int i = 0; i < node->child_count && offset < max; i++) {
        offset = pre_order_into(node->children[i], buf, max, offset);
    }

    return offset;
}

int tree_pre_order_str(const ReportNode *node, char *buf, int max) {
    if (!node || !buf || max <= 0) return 0;
    buf[0] = '\0';
    return pre_order_into(node, buf, max, 0);
}
