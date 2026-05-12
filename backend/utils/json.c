/*
 * json.c -- JSON builder and field extractor helpers.
 */

#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Parsing helpers ────────────────────────────────────────────── */

int json_get_str(struct mg_str body, const char *path,
                 char *out, size_t max) {
    struct mg_str val = mg_json_get_str(body, path);
    if (val.buf == NULL || val.len == 0) return -1;
    if (val.len >= max) return -1;
    memcpy(out, val.buf, val.len);
    out[val.len] = '\0';
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

/* ── Building helpers ────────────────────────────────────────────── */

void json_escape(const char *in, char *out, size_t max) {
    size_t j = 0;
    for (size_t i = 0; in[i] && j + 2 < max; i++) {
        if (in[i] == '"' || in[i] == '\\') {
            if (j + 3 >= max) break;
            out[j++] = '\\';
        }
        out[j++] = in[i];
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
