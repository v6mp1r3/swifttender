#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdint.h>
#include <stddef.h>

/*
 * ==================================================================
 *  SwiftTender -- Hash Table (Open Addressing + Linear Probing)
 *  dsa/hash_table.h
 * ==================================================================
 *
 * PURPOSE
 * -------
 * O(1) average lookup/insert/delete for string-keyed data.
 * Used in SwiftTender for:
 *   1. Session registry  -- token  -> User*  (auth on every request)
 *   2. User IDNO index   -- IDNO   -> User*  (duplicate-check on register)
 *
 * DESIGN: OPEN ADDRESSING + LINEAR PROBING
 * -----------------------------------------
 * All entries live in one flat array. Collision resolution:
 *   index = hash(key) % capacity
 *   if occupied: try index+1, index+2, ... (wrapping at capacity)
 *
 * vs. chaining: better cache locality, no per-entry malloc.
 *
 * TOMBSTONE DELETION
 * ------------------
 * We cannot zero out a deleted slot -- it would break probe chains.
 * Deleted slots become TOMBSTONE: skipped on lookup, reused on insert.
 *
 * LOAD FACTOR & RESIZE
 * --------------------
 * Load factor = (occupied + tombstones) / capacity.
 * When it exceeds 0.70, we rebuild at 2x capacity (all live entries
 * re-inserted). This keeps average probe length near 1.
 *
 * HASH FUNCTION: djb2 (Dan Bernstein)
 * ------------------------------------
 *   hash = 5381
 *   for each char c: hash = hash*33 ^ c
 * Fast, good distribution for short ASCII strings.
 *
 * COMPLEXITY
 * ----------
 *   insert / lookup / delete : O(1) avg,  O(n) worst
 *   resize                   : O(n)  (amortised O(1) per insert)
 */

#define HT_INITIAL_CAPACITY  128    /* must be power of 2            */
#define HT_MAX_KEY_LEN       256    /* max key string length         */
#define HT_LOAD_FACTOR_MAX   0.70f  /* resize trigger threshold      */

typedef enum {
    SLOT_EMPTY     = 0,
    SLOT_OCCUPIED  = 1,
    SLOT_TOMBSTONE = 2
} SlotState;

typedef struct {
    SlotState state;
    char      key[HT_MAX_KEY_LEN];
    void     *value;
} HSlot;

typedef struct {
    HSlot  *slots;
    size_t  capacity;
    size_t  count;       /* live entries only */
    size_t  tombstones;
} HashTable;

/* Lifecycle */
HashTable *ht_create(void);
void       ht_destroy(HashTable *ht);

/* Core operations */
int    ht_insert(HashTable *ht, const char *key, void *value);
void  *ht_lookup(HashTable *ht, const char *key);
int    ht_delete(HashTable *ht, const char *key);

/* Utility */
size_t ht_count(const HashTable *ht);
void   ht_print_stats(const HashTable *ht);

#endif /* HASH_TABLE_H */
