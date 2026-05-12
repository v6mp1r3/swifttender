#ifndef UPLOAD_H
#define UPLOAD_H

#include "mongoose.h"

/*
 * upload.h -- multipart file upload handling.
 *
 * Saves uploaded files to data/uploads/{user_id}/{original_name}.
 * Returns the saved path for storage in Document records.
 */

/*
 * upload_save -- extract and save a file from a multipart request.
 *
 * Parameters:
 *   hm       : incoming HTTP message (multipart body)
 *   field    : multipart field name (e.g. "file")
 *   user_id  : owner user ID (used to build directory path)
 *   out_path : buffer to receive the saved file path
 *   path_max : size of out_path buffer
 *
 * Returns 0 on success, -1 on failure.
 */
int upload_save(struct mg_http_message *hm, const char *field,
                uint32_t user_id, char *out_path, size_t path_max);

/* Ensure the upload directory for a user exists. */
int upload_ensure_dir(uint32_t user_id);

#endif /* UPLOAD_H */
