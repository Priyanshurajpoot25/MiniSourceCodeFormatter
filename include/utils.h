/**
 * @file utils.h
 * @brief Utility functions for memory management, string manipulation,
 *        dynamic string building, and terminal UI formatting.
 *
 * Part of Mini Source Code Formatter Using C Language.
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

/* ANSI Terminal Color Codes */
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_DIM     "\033[2m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_GRAY    "\033[90m"

/**
 * @struct StringBuilder
 * @brief Dynamic string accumulator that automatically expands its buffer.
 */
typedef struct {
    char *data;       /**< Pointer to heap-allocated character buffer */
    size_t length;    /**< Current string length (excluding null terminator) */
    size_t capacity;  /**< Allocated capacity */
} StringBuilder;

/* Dynamic Memory Helpers */
void *safe_malloc(size_t size);
void *safe_realloc(void *ptr, size_t size);
char *safe_strdup(const char *str);

/* StringBuilder Functions */
void sb_init(StringBuilder *sb);
void sb_init_with_capacity(StringBuilder *sb, size_t initial_cap);
void sb_append_char(StringBuilder *sb, char c);
void sb_append_str(StringBuilder *sb, const char *str);
void sb_append_strn(StringBuilder *sb, const char *str, size_t n);
void sb_append_format(StringBuilder *sb, const char *fmt, ...);
void sb_append_indent(StringBuilder *sb, int indent_level, int indent_size, bool use_tabs);
char *sb_to_string(StringBuilder *sb);
void sb_free(StringBuilder *sb);
void sb_clear(StringBuilder *sb);
char sb_last_char(const StringBuilder *sb);
void sb_trim_trailing_whitespace(StringBuilder *sb);

/* String Manipulation Helpers */
char *str_trim(char *str);
bool str_starts_with(const char *str, const char *prefix);
bool str_ends_with(const char *str, const char *suffix);
bool str_is_blank(const char *str);

/* Terminal & UI Helpers */
void terminal_enable_colors(bool enable);
bool terminal_colors_enabled(void);
void terminal_clear(void);
void terminal_pause(void);
bool get_user_line(char *buffer, size_t max_len);
int get_user_choice(int min_val, int max_val);
void print_banner(void);
void print_header(const char *title);
void print_separator(char ch, int length);
void print_success(const char *fmt, ...);
void print_warning(const char *fmt, ...);
void print_error(const char *fmt, ...);
void print_info(const char *fmt, ...);

/* Code Display Helpers */
void display_code_with_line_numbers(const char *code, const char *title);
void display_side_by_side_diff(const char *orig, const char *formatted);

#endif /* UTILS_H */
