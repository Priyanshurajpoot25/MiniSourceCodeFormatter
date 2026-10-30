/**
 * @file statistics.h
 * @brief Source code metrics and analysis: line counts, comments, functions,
 *        and file size comparisons.
 *
 * Part of Mini Source Code Formatter Using C Language.
 */

#ifndef STATISTICS_H
#define STATISTICS_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @struct CodeStatistics
 * @brief Metric counters for source code files.
 */
typedef struct {
    size_t total_lines;      /**< Total line count */
    size_t code_lines;       /**< Lines containing executable code */
    size_t blank_lines;      /**< Lines containing only whitespace */
    size_t comment_lines;    /**< Lines containing comments */
    size_t function_count;   /**< Estimated count of function definitions */
    size_t char_count;       /**< Total characters */
    long file_size_bytes;    /**< File size in bytes */
} CodeStatistics;

/* Core Statistics Calculation */
CodeStatistics calculate_code_statistics(const char *source, long file_size);

/* Display Functions */
void print_code_statistics(const CodeStatistics *stats, const char *title);
void print_statistics_comparison(const CodeStatistics *orig, const CodeStatistics *formatted,
                                const char *orig_label, const char *fmt_label);

#endif /* STATISTICS_H */
