/*
 * json.c -- JSON builder and field extractor helpers.
 *
 * Uses current mongoose JSON API:
 *   mg_json_get_str()  returns char* (heap-allocated, caller must free)
 *   mg_json_get_num()  returns bool, writes into double*
 */

#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Parsing helpers ────────────────────────────────────────────── */

int json_get_str(struct mg_str body, const char *path,
                 char *out, size_t max) {
    /* mg_json_get_str allocates a null-terminated string; caller frees */
    char *val = mg_json_get_str(body, path);
    if (!val) return -1;
    strncpy(out, val, max - 1);
    out[max - 1] = '\0';
    free(val);
    return 0;
}

int json_get_long(struct mg_str body, const char *path, long *out) {
    double d;
    if (!mg_json_get_num(body, path, &d)) return -1;
    *out = (long)d;
    return 0;
}

int json_get_float(struct mg_str body, const char *path, float *out) {
    double d;
    if (!mg_json_get_num(body, path, &d)) return -1;
    *out = (float)d;
    return 0;
}

/* ── Building helpers ─────────────────────────────────────────── */

void json_escape(const char *in, char *out, size_t max) {
    size_t j = 0;
    for (size_t i = 0; in[i] && j + 4 < max; i++) {
        unsigned char c = (unsigned char)in[i];
        if (c == '"' || c == '\\') {
            out[j++] = '\\'; out[j++] = (char)c;
        } else if (c == '\n') {
            out[j++] = '\\'; out[j++] = 'n';
        } else if (c == '\r') {
            out[j++] = '\\'; out[j++] = 'r';
        } else if (c == '\t') {
            out[j++] = '\\'; out[j++] = 't';
        } else {
            out[j++] = (char)c;
        }
    }
    out[j] = '\0';
}

int jb_str(char *buf, int max, const char *key, const char *val) {
    char escaped[1024];
    json_escape(val ? val : "", escaped, sizeof(escaped));
    return snprintf(buf, (size_t)max, "\"%s\":\"%s\",", key, escaped);
}

int jb_long(char *buf, int max, const char *key, long val) {
    return snprintf(buf, (size_t)max, "\"%s\":%ld,", key, val);
}

int jb_float(char *buf, int max, const char *key, float val) {
    return snprintf(buf, (size_t)max, "\"%s\":%.2f,", key, (double)val);
}

int jb_bool(char *buf, int max, const char *key, int val) {
    return snprintf(buf, (size_t)max, "\"%s\":%s,",
                    key, val ? "true" : "false");
}
