/*
 * upload.c -- multipart file upload handling.
 */

#include "upload.h"
#include "file_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

extern char s_data_dir[512];  /* defined in file_io.c -- shared data dir */

/* Build upload directory path for a user */
static void upload_dir(uint32_t user_id, char *out, size_t max) {
    snprintf(out, max, "./data/uploads/%u", user_id);
}

int upload_ensure_dir(uint32_t user_id) {
    char dir[512];
    upload_dir(user_id, dir, sizeof(dir));
    struct stat st;
    if (stat(dir, &st) == 0) return 0;
    return mkdir(dir, 0755);
}

int upload_save(struct mg_http_message *hm, const char *field,
                uint32_t user_id, char *out_path, size_t path_max) {
    if (!hm || !field || !out_path) return -1;

    /* Extract the file content from the multipart body */
    struct mg_http_part part;
    size_t pos = 0;
    int found  = 0;

    while ((pos = mg_http_next_multipart(hm->body, pos, &part)) > 0) {
        if (mg_strcmp(part.name, mg_str(field)) == 0) {
            found = 1;
            break;
        }
    }

    if (!found || part.body.len == 0) return -1;

    /* Ensure upload directory exists */
    upload_ensure_dir(user_id);

    /* Build a unique filename: timestamp_originalname */
    char dir[512];
    upload_dir(user_id, dir, sizeof(dir));

    /* Extract original filename from Content-Disposition header if available */
    char fname[256] = "upload";
    if (part.filename.len > 0 && part.filename.len < 255) {
        memcpy(fname, part.filename.buf, part.filename.len);
        fname[part.filename.len] = '\0';
    }

    snprintf(out_path, path_max, "%s/%lu_%s",
             dir, (unsigned long)time(NULL), fname);

    /* Write file contents to disk */
    FILE *f = fopen(out_path, "wb");
    if (!f) return -1;
    fwrite(part.body.buf, 1, part.body.len, f);
    fclose(f);

    fio_audit_append(user_id, "UPLOAD", 0, out_path);
    return 0;
}
