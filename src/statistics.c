/**
 * @file statistics.c
 * @brief Implementation of code metrics and comparative statistics.
 *
 * Part of Mini Source Code Formatter Using C Language.
 */

#include "statistics.h"
#include "tokenizer.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static size_t count_function_definitions(const TokenList *list) {
    if (!list) return 0;
    size_t count = 0;
    int brace_depth = 0;

    for (size_t i = 0; i < list->count; ++i) {
        const Token *tok = &list->tokens[i];

        if (tok->type == TOKEN_PUNCTUATION) {
            if (strcmp(tok->value, "{") == 0) {
                brace_depth++;
            } else if (strcmp(tok->value, "}") == 0) {
                brace_depth--;
                if (brace_depth < 0) brace_depth = 0;
            }
        }

        /* Detect function definition at top level (brace_depth == 0) */
        if (brace_depth == 0 && tok->type == TOKEN_IDENTIFIER) {
            /* Check if not a control keyword */
            if (strcmp(tok->value, "if") == 0 ||
                strcmp(tok->value, "for") == 0 ||
                strcmp(tok->value, "while") == 0 ||
                strcmp(tok->value, "switch") == 0) {
                continue;
            }

            /* Lookahead for ( ... ) followed by { */
            if (i + 1 < list->count && list->tokens[i + 1].type == TOKEN_PUNCTUATION &&
                strcmp(list->tokens[i + 1].value, "(") == 0) {
                /* Find matching closing parenthesis */
                int pdepth = 1;
                size_t j = i + 2;
                while (j < list->count && pdepth > 0) {
                    if (list->tokens[j].type == TOKEN_PUNCTUATION) {
                        if (strcmp(list->tokens[j].value, "(") == 0) pdepth++;
                        else if (strcmp(list->tokens[j].value, ")") == 0) pdepth--;
                    }
                    j++;
                }

                /* After closing parenthesis, skip any attributes/const and look for { */
                while (j < list->count && list->tokens[j].type != TOKEN_PUNCTUATION &&
                       list->tokens[j].type != TOKEN_PREPROCESSOR && list->tokens[j].type != TOKEN_EOF) {
                    j++;
                }

                if (j < list->count && list->tokens[j].type == TOKEN_PUNCTUATION &&
                    strcmp(list->tokens[j].value, "{") == 0) {
                    count++;
                }
            }
        }
    }
    return count;
}

CodeStatistics calculate_code_statistics(const char *source, long file_size) {
    CodeStatistics stats;
    memset(&stats, 0, sizeof(stats));

    if (!source) {
        stats.file_size_bytes = file_size >= 0 ? file_size : 0;
        return stats;
    }

    stats.char_count = strlen(source);
    stats.file_size_bytes = (file_size >= 0) ? file_size : (long)stats.char_count;

    /* Line-by-line analysis */
    const char *p = source;
    bool in_block_comment = false;

    while (*p) {
        stats.total_lines++;
        const char *line_start = p;
        while (*p && *p != '\n') {
            p++;
        }

        size_t line_len = (size_t)(p - line_start);
        if (line_len > 0 && line_start[line_len - 1] == '\r') {
            line_len--;
        }

        bool is_blank = true;
        bool has_comment = false;
        bool has_code = false;

        const char *cur = line_start;
        const char *line_end = line_start + line_len;

        while (cur < line_end) {
            if (in_block_comment) {
                has_comment = true;
                if (cur + 1 < line_end && *cur == '*' && *(cur + 1) == '/') {
                    in_block_comment = false;
                    cur += 2;
                    continue;
                }
                cur++;
            } else {
                if (isspace((unsigned char)*cur)) {
                    cur++;
                    continue;
                }
                is_blank = false;

                if (cur + 1 < line_end && *cur == '/' && *(cur + 1) == '/') {
                    has_comment = true;
                    break; /* Rest of line is comment */
                } else if (cur + 1 < line_end && *cur == '/' && *(cur + 1) == '*') {
                    has_comment = true;
                    in_block_comment = true;
                    cur += 2;
                } else {
                    has_code = true;
                    cur++;
                }
            }
        }

        if (is_blank) {
            stats.blank_lines++;
        } else {
            if (has_comment) stats.comment_lines++;
            if (has_code) stats.code_lines++;
        }

        if (*p == '\n') {
            p++;
        }
    }

    /* Tokenize to count functions */
    TokenList *tokens = tokenize_source(source, NULL, 0);
    if (tokens) {
        stats.function_count = count_function_definitions(tokens);
        token_list_free(tokens);
    }

    return stats;
}

void print_code_statistics(const CodeStatistics *stats, const char *title) {
    if (!stats) return;

    printf("\n%s=== %s ===%s\n", COLOR_CYAN, title ? title : "Code Statistics", COLOR_RESET);
    printf("  Total Lines:         %s%zu%s\n", COLOR_BOLD, stats->total_lines, COLOR_RESET);
    printf("  Executable Lines:    %s%zu%s\n", COLOR_BOLD, stats->code_lines, COLOR_RESET);
    printf("  Blank Lines:         %s%zu%s\n", COLOR_BOLD, stats->blank_lines, COLOR_RESET);
    printf("  Comment Lines:       %s%zu%s\n", COLOR_BOLD, stats->comment_lines, COLOR_RESET);
    printf("  Functions Detected:  %s%zu%s\n", COLOR_BOLD, stats->function_count, COLOR_RESET);
    printf("  Character Count:     %s%zu%s\n", COLOR_BOLD, stats->char_count, COLOR_RESET);
    printf("  File Size:           %s%ld bytes%s\n", COLOR_BOLD, stats->file_size_bytes, COLOR_RESET);
    print_separator('-', 40);
}

void print_statistics_comparison(const CodeStatistics *orig, const CodeStatistics *formatted,
                                const char *orig_label, const char *fmt_label) {
    if (!orig || !formatted) return;

    printf("\n%s========================================================================%s\n", COLOR_CYAN, COLOR_RESET);
    printf("                     SOURCE CODE METRICS COMPARISON                     \n");
    printf("%s========================================================================%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%-24s | %-14s | %-14s | %-12s\n",
           "Metric",
           orig_label ? orig_label : "Original",
           fmt_label ? fmt_label : "Formatted",
           "Delta");
    print_separator('-', 72);

    long line_delta = (long)formatted->total_lines - (long)orig->total_lines;
    long blank_delta = (long)formatted->blank_lines - (long)orig->blank_lines;
    long comment_delta = (long)formatted->comment_lines - (long)orig->comment_lines;
    long char_delta = (long)formatted->char_count - (long)orig->char_count;
    long size_delta = formatted->file_size_bytes - orig->file_size_bytes;

    printf("%-24s | %-14zu | %-14zu | %+ld\n", "Total Lines", orig->total_lines, formatted->total_lines, line_delta);
    printf("%-24s | %-14zu | %-14zu | %+ld\n", "Blank Lines", orig->blank_lines, formatted->blank_lines, blank_delta);
    printf("%-24s | %-14zu | %-14zu | %+ld\n", "Comment Lines", orig->comment_lines, formatted->comment_lines, comment_delta);
    printf("%-24s | %-14zu | %-14zu | %ld\n", "Functions Count", orig->function_count, formatted->function_count, (long)formatted->function_count - (long)orig->function_count);
    printf("%-24s | %-14zu | %-14zu | %+ld\n", "Character Count", orig->char_count, formatted->char_count, char_delta);
    printf("%-24s | %-14ld | %-14ld | %+ld bytes\n", "File Size (bytes)", orig->file_size_bytes, formatted->file_size_bytes, size_delta);
    print_separator('-', 72);

    if (orig->file_size_bytes > 0) {
        double pct = ((double)size_delta / (double)orig->file_size_bytes) * 100.0;
        printf("Size change: %s%+.2f%%%s due to formatting, whitespace normalization, and indent structure.\n",
               COLOR_BOLD, pct, COLOR_RESET);
    }
    printf("%s========================================================================%s\n", COLOR_CYAN, COLOR_RESET);
}
