/**
 * @file utils.c
 * @brief Implementation of utility functions for memory management,
 *        dynamic strings, string manipulation, and terminal UI.
 *
 * Part of Mini Source Code Formatter Using C Language.
 */

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <io.h>
    #define isatty _isatty
    #define fileno _fileno
#else
    #include <unistd.h>
#endif

static bool g_colors_enabled = true;

/* ========================================================================= */
/* Memory Allocation Helpers                                                 */
/* ========================================================================= */

void *safe_malloc(size_t size) {
    if (size == 0) {
        size = 1;
    }
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, COLOR_RED "FATAL: Memory allocation failed for %zu bytes." COLOR_RESET "\n", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *safe_realloc(void *ptr, size_t size) {
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        fprintf(stderr, COLOR_RED "FATAL: Memory reallocation failed for %zu bytes." COLOR_RESET "\n", size);
        free(ptr);
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

char *safe_strdup(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char *copy = (char *)safe_malloc(len + 1);
    memcpy(copy, str, len + 1);
    return copy;
}

/* ========================================================================= */
/* StringBuilder Implementation                                              */
/* ========================================================================= */

#define SB_INITIAL_CAPACITY 128

void sb_init(StringBuilder *sb) {
    sb_init_with_capacity(sb, SB_INITIAL_CAPACITY);
}

void sb_init_with_capacity(StringBuilder *sb, size_t initial_cap) {
    if (!sb) return;
    if (initial_cap < 16) initial_cap = 16;
    sb->data = (char *)safe_malloc(initial_cap);
    sb->data[0] = '\0';
    sb->length = 0;
    sb->capacity = initial_cap;
}

static void sb_ensure_capacity(StringBuilder *sb, size_t additional) {
    if (!sb) return;
    size_t required = sb->length + additional + 1;
    if (required > sb->capacity) {
        size_t new_cap = sb->capacity * 2;
        if (new_cap < required) {
            new_cap = required + 64;
        }
        sb->data = (char *)safe_realloc(sb->data, new_cap);
        sb->capacity = new_cap;
    }
}

void sb_append_char(StringBuilder *sb, char c) {
    if (!sb) return;
    sb_ensure_capacity(sb, 1);
    sb->data[sb->length++] = c;
    sb->data[sb->length] = '\0';
}

void sb_append_str(StringBuilder *sb, const char *str) {
    if (!sb || !str) return;
    size_t len = strlen(str);
    sb_ensure_capacity(sb, len);
    memcpy(sb->data + sb->length, str, len);
    sb->length += len;
    sb->data[sb->length] = '\0';
}

void sb_append_strn(StringBuilder *sb, const char *str, size_t n) {
    if (!sb || !str || n == 0) return;
    sb_ensure_capacity(sb, n);
    memcpy(sb->data + sb->length, str, n);
    sb->length += n;
    sb->data[sb->length] = '\0';
}

void sb_append_format(StringBuilder *sb, const char *fmt, ...) {
    if (!sb || !fmt) return;
    va_list args;
    va_start(args, fmt);
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (needed > 0) {
        sb_ensure_capacity(sb, (size_t)needed);
        vsnprintf(sb->data + sb->length, (size_t)needed + 1, fmt, args_copy);
        sb->length += (size_t)needed;
    }
    va_end(args_copy);
}

void sb_append_indent(StringBuilder *sb, int indent_level, int indent_size, bool use_tabs) {
    if (!sb || indent_level <= 0) return;
    if (use_tabs) {
        for (int i = 0; i < indent_level; ++i) {
            sb_append_char(sb, '\t');
        }
    } else {
        int total_spaces = indent_level * indent_size;
        for (int i = 0; i < total_spaces; ++i) {
            sb_append_char(sb, ' ');
        }
    }
}

char *sb_to_string(StringBuilder *sb) {
    if (!sb || !sb->data) return NULL;
    return safe_strdup(sb->data);
}

void sb_free(StringBuilder *sb) {
    if (sb) {
        free(sb->data);
        sb->data = NULL;
        sb->length = 0;
        sb->capacity = 0;
    }
}

void sb_clear(StringBuilder *sb) {
    if (sb && sb->data) {
        sb->data[0] = '\0';
        sb->length = 0;
    }
}

char sb_last_char(const StringBuilder *sb) {
    if (!sb || sb->length == 0) return '\0';
    return sb->data[sb->length - 1];
}

void sb_trim_trailing_whitespace(StringBuilder *sb) {
    if (!sb || sb->length == 0) return;
    while (sb->length > 0) {
        char c = sb->data[sb->length - 1];
        if (c == ' ' || c == '\t' || c == '\r') {
            sb->length--;
            sb->data[sb->length] = '\0';
        } else {
            break;
        }
    }
}

/* ========================================================================= */
/* String Manipulation Helpers                                               */
/* ========================================================================= */

char *str_trim(char *str) {
    if (!str) return NULL;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

bool str_starts_with(const char *str, const char *prefix) {
    if (!str || !prefix) return false;
    size_t len_pre = strlen(prefix);
    size_t len_str = strlen(str);
    return len_str >= len_pre && strncmp(str, prefix, len_pre) == 0;
}

bool str_ends_with(const char *str, const char *suffix) {
    if (!str || !suffix) return false;
    size_t len_suf = strlen(suffix);
    size_t len_str = strlen(str);
    if (len_str < len_suf) return false;
    return strcmp(str + len_str - len_suf, suffix) == 0;
}

bool str_is_blank(const char *str) {
    if (!str) return true;
    while (*str) {
        if (!isspace((unsigned char)*str)) return false;
        str++;
    }
    return true;
}

/* ========================================================================= */
/* Terminal & UI Functions                                                   */
/* ========================================================================= */

void terminal_enable_colors(bool enable) {
    g_colors_enabled = enable;
}

bool terminal_colors_enabled(void) {
    return g_colors_enabled;
}

void terminal_clear(void) {
#if defined(_WIN32) || defined(_WIN64)
    system("cls");
#else
    printf("\033[2J\033[H");
    fflush(stdout);
#endif
}

void terminal_pause(void) {
    printf("\n%sPress [Enter] to continue...%s", COLOR_GRAY, COLOR_RESET);
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

bool get_user_line(char *buffer, size_t max_len) {
    if (!buffer || max_len == 0) return false;
    if (!fgets(buffer, (int)max_len, stdin)) {
        return false;
    }
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    return true;
}

int get_user_choice(int min_val, int max_val) {
    char buf[64];
    while (1) {
        printf("%sEnter your choice (%d-%d): %s", COLOR_BOLD, min_val, max_val, COLOR_RESET);
        fflush(stdout);
        if (!get_user_line(buf, sizeof(buf))) {
            return -1; /* EOF encountered */
        }
        char *trimmed = str_trim(buf);
        if (strlen(trimmed) == 0) {
            continue;
        }
        char *endptr = NULL;
        long choice = strtol(trimmed, &endptr, 10);
        if (endptr != trimmed && *endptr == '\0' && choice >= min_val && choice <= max_val) {
            return (int)choice;
        }
        printf("%sInvalid choice. Please enter a number between %d and %d.%s\n", COLOR_RED, min_val, max_val, COLOR_RESET);
    }
}

void print_separator(char ch, int length) {
    for (int i = 0; i < length; ++i) {
        putchar(ch);
    }
    putchar('\n');
}

void print_banner(void) {
    printf("%s", COLOR_CYAN);
    print_separator('=', 50);
    printf("         MINI SOURCE CODE FORMATTER\n");
    printf("               C LANGUAGE\n");
    print_separator('=', 50);
    printf("%s", COLOR_RESET);
}

void print_header(const char *title) {
    printf("\n%s--- %s ---%s\n", COLOR_CYAN, title, COLOR_RESET);
}

void print_success(const char *fmt, ...) {
    printf("%s[SUCCESS]%s ", COLOR_GREEN, COLOR_RESET);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

void print_warning(const char *fmt, ...) {
    printf("%s[WARNING]%s ", COLOR_YELLOW, COLOR_RESET);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

void print_error(const char *fmt, ...) {
    printf("%s[ERROR]%s ", COLOR_RED, COLOR_RESET);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

void print_info(const char *fmt, ...) {
    printf("%s[INFO]%s ", COLOR_BLUE, COLOR_RESET);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

/* ========================================================================= */
/* Code Display Functions                                                    */
/* ========================================================================= */

void display_code_with_line_numbers(const char *code, const char *title) {
    if (!code) {
        print_warning("No code to display.");
        return;
    }
    printf("\n%s=== %s ===%s\n", COLOR_BOLD, title ? title : "Code Preview", COLOR_RESET);
    int line_num = 1;
    const char *p = code;
    printf("%s%4d | %s", COLOR_GRAY, line_num, COLOR_RESET);
    while (*p) {
        if (*p == '\n') {
            line_num++;
            putchar('\n');
            if (*(p + 1) != '\0') {
                printf("%s%4d | %s", COLOR_GRAY, line_num, COLOR_RESET);
            }
        } else {
            putchar(*p);
        }
        p++;
    }
    printf("\n%s----------------------------------------%s\n", COLOR_GRAY, COLOR_RESET);
}

static void split_lines(const char *text, char ***out_lines, int *out_count) {
    if (!text) {
        *out_lines = NULL;
        *out_count = 0;
        return;
    }
    int cap = 64;
    int count = 0;
    char **lines = (char **)safe_malloc(sizeof(char *) * cap);

    const char *start = text;
    const char *p = text;
    while (*p) {
        if (*p == '\n') {
            size_t len = (size_t)(p - start);
            if (len > 0 && start[len - 1] == '\r') len--;
            char *line = (char *)safe_malloc(len + 1);
            memcpy(line, start, len);
            line[len] = '\0';
            if (count >= cap) {
                cap *= 2;
                lines = (char **)safe_realloc(lines, sizeof(char *) * cap);
            }
            lines[count++] = line;
            start = p + 1;
        }
        p++;
    }
    if (p > start || count == 0) {
        size_t len = (size_t)(p - start);
        if (len > 0 && start[len - 1] == '\r') len--;
        char *line = (char *)safe_malloc(len + 1);
        memcpy(line, start, len);
        line[len] = '\0';
        if (count >= cap) {
            cap *= 2;
            lines = (char **)safe_realloc(lines, sizeof(char *) * cap);
        }
        lines[count++] = line;
    }
    *out_lines = lines;
    *out_count = count;
}

static void free_lines(char **lines, int count) {
    if (!lines) return;
    for (int i = 0; i < count; ++i) {
        free(lines[i]);
    }
    free(lines);
}

void display_side_by_side_diff(const char *orig, const char *formatted) {
    char **orig_lines = NULL;
    int orig_count = 0;
    char **fmt_lines = NULL;
    int fmt_count = 0;

    split_lines(orig, &orig_lines, &orig_count);
    split_lines(formatted, &fmt_lines, &fmt_count);

    int max_rows = orig_count > fmt_count ? orig_count : fmt_count;
    const int col_width = 46;

    printf("\n%s%-6s %-*s | %-6s %-*s%s\n",
           COLOR_BOLD, "LINE", col_width, "ORIGINAL CODE", "LINE", col_width, "FORMATTED CODE", COLOR_RESET);
    print_separator('-', col_width * 2 + 16);

    for (int i = 0; i < max_rows; ++i) {
        char left_buf[128] = "";
        char right_buf[128] = "";
        char left_no[16] = "";
        char right_no[16] = "";

        if (i < orig_count) {
            snprintf(left_no, sizeof(left_no), "%4d", i + 1);
            strncpy(left_buf, orig_lines[i], col_width);
            left_buf[col_width] = '\0';
        }
        if (i < fmt_count) {
            snprintf(right_no, sizeof(right_no), "%4d", i + 1);
            strncpy(right_buf, fmt_lines[i], col_width);
            right_buf[col_width] = '\0';
        }

        bool match = (i < orig_count && i < fmt_count && strcmp(orig_lines[i], fmt_lines[i]) == 0);
        const char *color = match ? COLOR_RESET : COLOR_CYAN;

        printf("%s%s%-6s %-*s%s | %s%-6s %-*s%s\n",
               color,
               COLOR_GRAY, left_no, col_width, left_buf, COLOR_RESET,
               COLOR_GRAY, right_no, col_width, right_buf, COLOR_RESET);
    }
    print_separator('-', col_width * 2 + 16);

    free_lines(orig_lines, orig_count);
    free_lines(fmt_lines, fmt_count);
}
