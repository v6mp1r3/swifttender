/*
 * ==================================================================
 *  SwiftTender -- Hash Table Implementation
 *  dsa/hash_table.c
 * ==================================================================
 */

#include "hash_table.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ----------------------------------------------------------------
 * djb2 hash function (Dan Bernstein)
 *
 * Classic algorithm: initialise hash to 5381, then for each byte:
 *     hash = hash * 33 XOR byte
 * The magic number 33 (2^5 + 1) gives good avalanche effect for
 * short ASCII strings.
 *
 * We mask the result to table capacity (which is always a power of
 * two) so we can use bitwise AND instead of expensive modulo:
 *     index = hash & (capacity - 1)   ==   hash % capacity
 * ---------------------------------------------------------------- */
static size_t djb2(const char *key, size_t capacity) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*key++)) {
        hash = ((hash << 5) + hash) ^ (unsigned long)c;  /* hash*33 ^ c */
    }
    return hash & (capacity - 1);   /* fast modulo for power-of-2 cap */
}

/* ----------------------------------------------------------------
 * ht_create -- allocate and initialise a hash table.
 *
 * calloc() zero-initialises the slots array, so all slots start
 * in SLOT_EMPTY state (enum value 0) without an explicit loop.
 * ---------------------------------------------------------------- */
HashTable *ht_create(void) {
    HashTable *ht = malloc(sizeof(HashTable));
    if (!ht) return NULL;

    ht->slots = calloc(HT_INITIAL_CAPACITY, sizeof(HSlot));
    if (!ht->slots) { free(ht); return NULL; }

    ht->capacity   = HT_INITIAL_CAPACITY;
    ht->count      = 0;
    ht->tombstones = 0;
    return ht;
}

/* ----------------------------------------------------------------
 * ht_destroy -- free all heap memory owned by the table.
 * ---------------------------------------------------------------- */
void ht_destroy(HashTable *ht) {
    if (!ht) return;
    free(ht->slots);
    free(ht);
}

/* ----------------------------------------------------------------
 * ht_resize -- internal: rebuild the table at a new capacity.
 *
 * All OCCUPIED entries are re-inserted into a fresh slot array.
 * TOMBSTONE slots are simply dropped (they become EMPTY in the new
 * array). This is why tombstone count resets to 0 after resize.
 *
 * Called automatically by ht_insert() when load factor > 0.70.
 * ---------------------------------------------------------------- */
static int ht_resize(HashTable *ht, size_t new_cap) {
    /* Allocate the new (zero-initialised) slot array */
    HSlot *new_slots = calloc(new_cap, sizeof(HSlot));
    if (!new_slots) return -1;

    /* Re-insert every live entry into the new array */
    for (size_t i = 0; i < ht->capacity; i++) {
        if (ht->slots[i].state != SLOT_OCCUPIED) continue;

        /* Find a free slot in the new array using the same probe logic */
        size_t idx = djb2(ht->slots[i].key, new_cap);
        while (new_slots[idx].state == SLOT_OCCUPIED) {
            idx = (idx + 1) & (new_cap - 1);
        }
        new_slots[idx] = ht->slots[i];   /* copy entire slot struct */
    }

    free(ht->slots);
    ht->slots      = new_slots;
    ht->capacity   = new_cap;
    ht->tombstones = 0;   /* tombstones are gone in the fresh array */
    return 0;
}

/* ----------------------------------------------------------------
 * ht_insert -- insert or update a key-value pair.
 *
 * Probe sequence (linear probing):
 *   1. Compute starting index with djb2.
 *   2. Walk forward until we find EMPTY, TOMBSTONE, or the same key.
 *   3. If TOMBSTONE found first, remember it as a reuse candidate but
 *      keep probing (the key might exist further along the chain).
 *   4. Insert at the tombstone slot if key not found further, otherwise
 *      update the existing slot.
 *
 * Resize check happens BEFORE insertion so we never insert into a
 * table that is already over the load factor threshold.
 * ---------------------------------------------------------------- */
int ht_insert(HashTable *ht, const char *key, void *value) {
    if (!ht || !key) return -1;

    /* Load factor = (live + tombstones) / capacity
     * We include tombstones because they still occupy probe chain slots. */
    float load = (float)(ht->count + ht->tombstones) / (float)ht->capacity;
    if (load >= HT_LOAD_FACTOR_MAX) {
        if (ht_resize(ht, ht->capacity * 2) != 0) return -1;
    }

    size_t idx        = djb2(key, ht->capacity);
    size_t tombstone  = (size_t)-1;   /* index of first tombstone found */
    size_t probes     = 0;

    while (probes < ht->capacity) {
        HSlot *slot = &ht->slots[idx];

        if (slot->state == SLOT_EMPTY) {
            /* Key definitely not in table -- insert here (or at tombstone) */
            size_t target = (tombstone != (size_t)-1) ? tombstone : idx;
            ht->slots[target].state = SLOT_OCCUPIED;
            strncpy(ht->slots[target].key, key, HT_MAX_KEY_LEN - 1);
            ht->slots[target].key[HT_MAX_KEY_LEN - 1] = '\0';
            ht->slots[target].value = value;
            ht->count++;
            if (tombstone != (size_t)-1) ht->tombstones--;
            return 0;
        }

        if (slot->state == SLOT_TOMBSTONE) {
            /* Remember first tombstone; continue probing for existing key */
            if (tombstone == (size_t)-1) tombstone = idx;
        } else {
            /* SLOT_OCCUPIED -- check if it is our key (update case) */
            if (strncmp(slot->key, key, HT_MAX_KEY_LEN) == 0) {
                slot->value = value;   /* overwrite existing value */
                return 0;
            }
        }

        idx = (idx + 1) & (ht->capacity - 1);   /* linear probe step */
        probes++;
    }

    /* Table is full even after resize attempt (should not happen) */
    return -1;
}

/* ----------------------------------------------------------------
 * ht_lookup -- find a value by key.
 *
 * Linear probe from the hash index. Stop when:
 *   - SLOT_EMPTY found  --> key not in table (probe chain is broken here)
 *   - Key matches       --> found, return value
 *   - SLOT_TOMBSTONE    --> skip (key might be further along)
 * ---------------------------------------------------------------- */
void *ht_lookup(HashTable *ht, const char *key) {
    if (!ht || !key) return NULL;

    size_t idx    = djb2(key, ht->capacity);
    size_t probes = 0;

    while (probes < ht->capacity) {
        HSlot *slot = &ht->slots[idx];

        if (slot->state == SLOT_EMPTY) {
            return NULL;   /* end of probe chain -- not found */
        }

        if (slot->state == SLOT_OCCUPIED &&
            strncmp(slot->key, key, HT_MAX_KEY_LEN) == 0) {
            return slot->value;   /* found */
        }

        /* TOMBSTONE or different key -- keep probing */
        idx = (idx + 1) & (ht->capacity - 1);
        probes++;
    }

    return NULL;   /* full table scan, not found */
}

/* ----------------------------------------------------------------
 * ht_delete -- mark a slot as TOMBSTONE (lazy deletion).
 *
 * We do NOT zero the slot because other keys may have been inserted
 * past this slot during a collision chain. Setting it to TOMBSTONE
 * preserves the probe chain for those keys while making the slot
 * available for future insertions.
 * ---------------------------------------------------------------- */
int ht_delete(HashTable *ht, const char *key) {
    if (!ht || !key) return -1;

    size_t idx    = djb2(key, ht->capacity);
    size_t probes = 0;

    while (probes < ht->capacity) {
        HSlot *slot = &ht->slots[idx];

        if (slot->state == SLOT_EMPTY) {
            return -1;   /* not found */
        }

        if (slot->state == SLOT_OCCUPIED &&
            strncmp(slot->key, key, HT_MAX_KEY_LEN) == 0) {
            slot->state = SLOT_TOMBSTONE;
            ht->count--;
            ht->tombstones++;
            return 0;   /* deleted */
        }

        idx = (idx + 1) & (ht->capacity - 1);
        probes++;
    }

    return -1;   /* not found */
}

/* ----------------------------------------------------------------
 * ht_count -- return number of live entries.
 * ---------------------------------------------------------------- */
size_t ht_count(const HashTable *ht) {
    return ht ? ht->count : 0;
}

/* ----------------------------------------------------------------
 * ht_print_stats -- debug helper: print table health to stdout.
 * ---------------------------------------------------------------- */
void ht_print_stats(const HashTable *ht) {
    if (!ht) { printf("[ht] NULL table\n"); return; }
    printf("[ht] capacity=%zu  live=%zu  tombstones=%zu  load=%.2f%%\n",
           ht->capacity,
           ht->count,
           ht->tombstones,
           100.0f * (float)(ht->count + ht->tombstones) / (float)ht->capacity);
}
