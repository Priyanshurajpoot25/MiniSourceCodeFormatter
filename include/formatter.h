/**
 * @file formatter.h
 * @brief Formatting engine configuration, diagnostics, and formatting routines.
 *
 * Part of Mini Source Code Formatter Using C Language.
 */

#ifndef FORMATTER_H
#define FORMATTER_H

#include "tokenizer.h"
#include <stdbool.h>
#include <stddef.h>

#define MAX_WARNINGS 32
#define MAX_WARNING_LEN 256

/**
 * @struct FormatterConfig
 * @brief User-configurable options for the code formatter.
 */
typedef struct {
    int indent_size;              /**< Number of spaces per indent level (default: 4) */
    bool use_tabs;                /**< Use '\t' instead of spaces (default: false) */
    bool space_around_binary;     /**< Put spaces around binary operators (default: true) */
    bool space_after_comma;       /**< Put space after commas (default: true) */
    bool space_after_keyword;     /**< Space after control keywords: if ( (default: true) */
    bool knr_braces;              /**< K&R style braces on same line (default: true) */
    bool remove_trailing_ws;      /**< Strip trailing spaces from lines (default: true) */
    int max_blank_lines;          /**< Maximum consecutive blank lines allowed (default: 1) */
} FormatterConfig;

/**
 * @struct FormatterDiagnostics
 * @brief Analysis results, syntax warnings, and balance checking.
 */
typedef struct {
    int brace_balance;            /**< Final brace balance (0 if balanced) */
    int paren_balance;            /**< Final parenthesis balance (0 if balanced) */
    int square_balance;           /**< Final bracket balance (0 if balanced) */
    int warning_count;            /**< Number of detected warnings */
    char warnings[MAX_WARNINGS][MAX_WARNING_LEN]; /**< Warning descriptions */
} FormatterDiagnostics;

/* Config Lifecycle */
FormatterConfig formatter_default_config(void);

/* Diagnostics Lifecycle */
void formatter_diagnostics_init(FormatterDiagnostics *diag);
void formatter_diagnostics_add_warning(FormatterDiagnostics *diag, const char *fmt, ...);
void formatter_diagnostics_print(const FormatterDiagnostics *diag);

/* Core Formatting Functions */
char *format_tokens(const TokenList *list, const FormatterConfig *config, FormatterDiagnostics *diag);
char *format_source_code(const char *source, const FormatterConfig *config, FormatterDiagnostics *diag);

#endif /* FORMATTER_H */
