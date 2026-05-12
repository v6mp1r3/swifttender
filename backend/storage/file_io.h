#ifndef FILE_IO_H
#define FILE_IO_H

/*
 * file_io.h -- binary persistence layer
 *
 * Full implementation in: feat: define all C structs and file I/O layer
 */

/* Initialise the data directory and create empty .bin files if missing.
 * Returns 0 on success, -1 on failure. */
int file_io_init(const char *data_dir);

#endif /* FILE_IO_H */
