/**
 * @file file_handler.c
 * @brief Implementation of file operations, backup routines, and validation.
 *
 * Part of Mini Source Code Formatter Using C Language.
 */

#include "file_handler.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <direct.h>
    #define mkdir_cross(path) _mkdir(path)
#else
    #define mkdir_cross(path) mkdir(path, 0755)
#endif

bool file_exists(const char *path) {
    if (!path || strlen(path) == 0) return false;
    struct stat st;
    if (stat(path, &st) == 0) {
        /* Check that it is a regular file, not a directory */
        return (st.st_mode & S_IFREG) != 0;
    }
    return false;
}

bool validate_file_extension(const char *path) {
    if (!path) return false;
    const char *dot = strrchr(path, '.');
    if (!dot) return false;
    return (strcmp(dot, ".c") == 0 ||
            strcmp(dot, ".h") == 0 ||
            strcmp(dot, ".cpp") == 0 ||
            strcmp(dot, ".hpp") == 0 ||
            strcmp(dot, ".cc") == 0 ||
            strcmp(dot, ".cxx") == 0);
}

long get_file_size_on_disk(const char *path) {
    if (!path) return -1;
    struct stat st;
    if (stat(path, &st) == 0) {
        return (long)st.st_size;
    }
    return -1;
}

char *read_file_to_string(const char *path, long *out_size, char *err_msg, size_t err_size) {
    if (err_msg && err_size > 0) err_msg[0] = '\0';
    if (out_size) *out_size = 0;

    if (!path || strlen(path) == 0) {
        if (err_msg && err_size > 0) snprintf(err_msg, err_size, "Path is empty or null.");
        return NULL;
    }

    if (!file_exists(path)) {
        if (err_msg && err_size > 0) snprintf(err_msg, err_size, "File '%s' does not exist or is not a regular file.", path);
        return NULL;
    }

    if (!validate_file_extension(path)) {
        if (err_msg && err_size > 0) snprintf(err_msg, err_size, "Warning: '%s' is not a standard C/C++ source or header file (.c, .h).", path);
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        if (err_msg && err_size > 0) snprintf(err_msg, err_size, "Failed to open file '%s' for reading.", path);
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        if (err_msg && err_size > 0) snprintf(err_msg, err_size, "Failed to determine file size for '%s'.", path);
        fclose(f);
        return NULL;
    }

    long size = ftell(f);
    if (size < 0) {
        if (err_msg && err_size > 0) snprintf(err_msg, err_size, "Invalid file length for '%s'.", path);
        fclose(f);
        return NULL;
    }

    rewind(f);

    /* Allocate buffer including space for null terminator */
    char *buffer = (char *)safe_malloc((size_t)size + 1);
    if (size > 0) {
        size_t bytes_read = fread(buffer, 1, (size_t)size, f);
        if (bytes_read != (size_t)size) {
            if (err_msg && err_size > 0) snprintf(err_msg, err_size, "Incomplete read of '%s' (%zu of %ld bytes).", path, bytes_read, size);
            free(buffer);
            fclose(f);
            return NULL;
        }
    }
    buffer[size] = '\0';
    fclose(f);

    if (out_size) *out_size = size;
    return buffer;
}

static void ensure_parent_directory(const char *file_path) {
    if (!file_path) return;
    char temp[512];
    strncpy(temp, file_path, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    char *last_slash = strrchr(temp, '/');
#if defined(_WIN32) || defined(_WIN64)
    char *last_bslash = strrchr(temp, '\\');
    if (!last_slash || (last_bslash && last_bslash > last_slash)) {
        last_slash = last_bslash;
    }
#endif

    if (last_slash) {
        *last_slash = '\0';
        mkdir_cross(temp);
    }
}

bool write_string_to_file(const char *path, const char *content, char *err_msg, size_t err_size) {
    if (err_msg && err_size > 0) err_msg[0] = '\0';

    if (!path || strlen(path) == 0) {
        if (err_msg && err_size > 0) snprintf(err_msg, err_size, "Target output path is empty.");
        return false;
    }

    ensure_parent_directory(path);

    FILE *f = fopen(path, "wb");
    if (!f) {
        if (err_msg && err_size > 0) snprintf(err_msg, err_size, "Cannot open file '%s' for writing.", path);
        return false;
    }

    if (content) {
        size_t len = strlen(content);
        size_t written = fwrite(content, 1, len, f);
        if (written != len) {
            if (err_msg && err_size > 0) snprintf(err_msg, err_size, "Failed to write complete content to '%s'.", path);
            fclose(f);
            return false;
        }
    }

    fclose(f);
    return true;
}

bool create_backup_file(const char *path, char *out_backup_path, size_t max_len, char *err_msg, size_t err_size) {
    if (!path || !file_exists(path)) {
        if (err_msg && err_size > 0) snprintf(err_msg, err_size, "Source file for backup does not exist: %s", path ? path : "null");
        return false;
    }

    char backup_name[512];
    snprintf(backup_name, sizeof(backup_name), "%s.bak", path);

    long size = 0;
    char *data = read_file_to_string(path, &size, err_msg, err_size);
    if (!data) {
        return false;
    }

    bool ok = write_string_to_file(backup_name, data, err_msg, err_size);
    free(data);

    if (ok && out_backup_path && max_len > 0) {
        strncpy(out_backup_path, backup_name, max_len - 1);
        out_backup_path[max_len - 1] = '\0';
    }
    return ok;
}

int batch_format_files(const char **file_paths, int count, const FormatterConfig *config, const char *output_dir) {
    if (!file_paths || count <= 0) return 0;

    int success_count = 0;
    print_header("Batch Source Code Formatting");
    printf("Processing %d file(s)...\n\n", count);

    for (int i = 0; i < count; ++i) {
        const char *input_path = file_paths[i];
        printf("[%d/%d] Processing '%s'...\n", i + 1, count, input_path);

        char err_msg[256] = "";
        long size = 0;
        char *source = read_file_to_string(input_path, &size, err_msg, sizeof(err_msg));
        if (!source) {
            print_error("Failed to read: %s", err_msg);
            continue;
        }

        FormatterDiagnostics diag;
        char *formatted = format_source_code(source, config, &diag);
        free(source);

        if (!formatted) {
            print_error("Failed to format '%s'", input_path);
            continue;
        }

        char out_path[512];
        if (output_dir && strlen(output_dir) > 0) {
            const char *base = strrchr(input_path, '/');
#if defined(_WIN32) || defined(_WIN64)
            const char *b_bslash = strrchr(input_path, '\\');
            if (!base || (b_bslash && b_bslash > base)) base = b_bslash;
#endif
            base = base ? base + 1 : input_path;
            snprintf(out_path, sizeof(out_path), "%s/%s", output_dir, base);
        } else {
            snprintf(out_path, sizeof(out_path), "%s.formatted.c", input_path);
        }

        if (write_string_to_file(out_path, formatted, err_msg, sizeof(err_msg))) {
            print_success("Saved formatted output to '%s'", out_path);
            if (diag.warning_count > 0) {
                print_warning("%d diagnostic warning(s) detected.", diag.warning_count);
            }
            success_count++;
        } else {
            print_error("Failed to save formatted file: %s", err_msg);
        }

        free(formatted);
    }

    printf("\nBatch formatting complete: %d of %d files formatted successfully.\n", success_count, count);
    return success_count;
}
