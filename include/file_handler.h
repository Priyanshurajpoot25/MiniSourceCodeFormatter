/**
 * @file file_handler.h
 * @brief File operations: reading, writing, path validation, extension checks,
 *        safe backups, and batch processing.
 *
 * Part of Mini Source Code Formatter Using C Language.
 */

#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include "formatter.h"
#include <stdbool.h>
#include <stddef.h>

/* File Validation */
bool file_exists(const char *path);
bool validate_file_extension(const char *path);
long get_file_size_on_disk(const char *path);

/* File I/O */
char *read_file_to_string(const char *path, long *out_size, char *err_msg, size_t err_size);
bool write_string_to_file(const char *path, const char *content, char *err_msg, size_t err_size);

/* Backup and Overwrite Safety */
bool create_backup_file(const char *path, char *out_backup_path, size_t max_len, char *err_msg, size_t err_size);

/* Batch Formatting */
int batch_format_files(const char **file_paths, int count, const FormatterConfig *config, const char *output_dir);

#endif /* FILE_HANDLER_H */
