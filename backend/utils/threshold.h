#ifndef THRESHOLD_H
#define THRESHOLD_H

/*
 * threshold.h -- Legal procurement threshold table.
 *
 * Loaded from data/thresholds.csv at startup (threshold_load).
 * Sorted by category code so binary search can be applied.
 * Values are in MDL excluding VAT (HG 870/2022, Law 131/2015).
 */

#define THRESHOLD_MAX_CATEGORIES 16
#define THRESHOLD_CODE_LEN        16
#define THRESHOLD_NAME_LEN        64

typedef struct {
    char  code[THRESHOLD_CODE_LEN];     /* e.g. "GOODS", "WORKS"    */
    char  name[THRESHOLD_NAME_LEN];     /* human-readable label      */
    float direct_invoice_limit;         /* below: just pay invoice   */
    float direct_contract_limit;        /* below: direct contract OK */
    float procedure_limit;              /* below: low-value procedure */
                                        /* above: must use MTender   */
} ThresholdCategory;

/* Load thresholds.csv into a sorted array. Returns count loaded. */
int threshold_load(const char *csv_path,
                   ThresholdCategory *out,
                   int max);

/* Determine procedure type for a given value + category code.
 * Returns: 0 = direct invoice, 1 = direct contract,
 *          2 = low-value procedure, 3 = must use MTender */
int threshold_classify(const ThresholdCategory *table, int n,
                       const char *code, float value);

#endif /* THRESHOLD_H */
