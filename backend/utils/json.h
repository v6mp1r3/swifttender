#ifndef JSON_H
#define JSON_H

#include <stddef.h>
#include "mongoose.h"

/*
 * json.h -- lightweight JSON builder and field extractor helpers.
 *
 * Mongoose already provides mg_json_get / mg_json_get_str for
 * parsing incoming request bodies. This module adds:
 *   - jb_*  helpers for building outgoing JSON strings
 *   - json_get_str / json_get_long for clean field extraction
 */

/* ── Parsing helpers (wrap mongoose) ───────────────────────────── */

/*
 * json_get_str -- extract a string field from a JSON body.
 *
 * Example: json_get_str(hm->body, "$.email", buf, sizeof(buf))
 * Returns 0 on success, -1 if field missing or too long.
 */
int json_get_str(struct mg_str body, const char *path,
                 char *out, size_t max);

/*
 * json_get_long -- extract a numeric field from a JSON body.
 * Returns 0 on success, -1 if field missing or not a number.
 */
int json_get_long(struct mg_str body, const char *path, long *out);

/*
 * json_get_float -- extract a float field from a JSON body.
 */
int json_get_float(struct mg_str body, const char *path, float *out);

/* ── Building helpers ───────────────────────────────────────────── */

/*
 * jb_str -- append  "key": "value"  to buf (with trailing comma).
 * jb_long -- append "key": 123
 * jb_float -- append "key": 123.45
 * jb_bool -- append "key": true|false
 *
 * All return number of bytes written, or 0 on overflow.
 * Use inside a manually assembled JSON object:
 *
 *   char buf[512];
 *   int  n = 0;
 *   n += snprintf(buf+n, sizeof(buf)-n, "{");
 *   n += jb_str (buf+n, sizeof(buf)-n, "name",  user.name);
 *   n += jb_long(buf+n, sizeof(buf)-n, "id",    user.id);
 *   // remove trailing comma, close object:
 *   if (buf[n-1] == ',') n--;
 *   n += snprintf(buf+n, sizeof(buf)-n, "}");
 */
int jb_str  (char *buf, int max, const char *key, const char *val);
int jb_long (char *buf, int max, const char *key, long val);
int jb_float(char *buf, int max, const char *key, float val);
int jb_bool (char *buf, int max, const char *key, int val);

/* Escape a string for safe JSON embedding (handles quotes, backslashes). */
void json_escape(const char *in, char *out, size_t max);

#endif /* JSON_H */
