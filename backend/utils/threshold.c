/*
 * threshold.c -- Load and classify procurement thresholds.
 * Uses bs_find_threshold() from dsa/binary_search.c for O(log n) lookup.
 */

#include "threshold.h"
#include "dsa/binary_search.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ----------------------------------------------------------------
 * compare_categories -- qsort comparator to sort by .code ascending.
 * Required precondition for binary search.
 * ---------------------------------------------------------------- */
static int compare_categories(const void *a, const void *b) {
    const ThresholdCategory *ca = (const ThresholdCategory *)a;
    const ThresholdCategory *cb = (const ThresholdCategory *)b;
    return strncmp(ca->code, cb->code, THRESHOLD_CODE_LEN);
}

/* ----------------------------------------------------------------
 * threshold_load -- parse thresholds.csv into a sorted array.
 *
 * CSV format (see data/thresholds.csv):
 *   code,name,direct_invoice_limit,direct_contract_limit,procedure_limit
 *   GOODS,Goods and services,50000,150000,300000
 *
 * Lines starting with '#' are comments and are skipped.
 * After loading, the array is sorted with qsort() so binary search
 * can be applied.
 *
 * Returns the number of categories loaded, or -1 on error.
 * ---------------------------------------------------------------- */
int threshold_load(const char *csv_path,
                   ThresholdCategory *out,
                   int max) {
    if (!csv_path || !out || max <= 0) return -1;

    FILE *f = fopen(csv_path, "r");
    if (!f) {
        fprintf(stderr, "[threshold] Cannot open %s\n", csv_path);
        return -1;
    }

    char line[256];
    int  count = 0;

    while (fgets(line, sizeof(line), f) && count < max) {
        /* Skip comment lines and blank lines */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;

        ThresholdCategory cat;
        memset(&cat, 0, sizeof(cat));

        /* Parse: code,name,invoice_limit,contract_limit,procedure_limit */
        if (sscanf(line, "%15[^,],%63[^,],%f,%f,%f",
                   cat.code,
                   cat.name,
                   &cat.direct_invoice_limit,
                   &cat.direct_contract_limit,
                   &cat.procedure_limit) == 5) {
            out[count++] = cat;
        }
    }
    fclose(f);

    /* Sort by code so bs_find_threshold() can be applied */
    qsort(out, (size_t)count, sizeof(ThresholdCategory), compare_categories);

    printf("[threshold] Loaded %d categories from %s\n", count, csv_path);
    return count;
}

/* ----------------------------------------------------------------
 * threshold_classify -- determine procedure type for a given value.
 *
 * Uses bs_find_threshold() (O(log n)) to find the category, then
 * compares the value against the three limit tiers.
 *
 * Returns:
 *   0 = direct invoice (below direct_invoice_limit)
 *   1 = direct contract (below direct_contract_limit)
 *   2 = low-value procedure (below procedure_limit) -- SwiftTender scope
 *   3 = must use MTender (above procedure_limit)
 *  -1 = category not found
 * ---------------------------------------------------------------- */
int threshold_classify(const ThresholdCategory *table, int n,
                       const char *code, float value) {
    const ThresholdCategory *cat = bs_find_threshold(table, n, code);
    if (!cat) return -1;

    if (value <= cat->direct_invoice_limit)  return 0;
    if (value <= cat->direct_contract_limit) return 1;
    if (value <= cat->procedure_limit)       return 2;
    return 3;   /* exceeds all limits -> MTender required */
}
