/**
 * @file main.c
 * @brief Interactive CLI menu and command-line entry point.
 *
 * Part of Mini Source Code Formatter Using C Language.
 */

#include "formatter.h"
#include "file_handler.h"
#include "statistics.h"
#include "tokenizer.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define VERSION "1.0.0"
#define DEFAULT_OUTPUT_PATH "output/formatted_output.c"

/* Application runtime context */
typedef struct {
    char source_path[512];
    char *original_code;
    long original_size;
    char *formatted_code;
    FormatterConfig config;
    FormatterDiagnostics diagnostics;
    bool is_formatted;
} AppContext;

static void init_app_context(AppContext *ctx) {
    memset(ctx, 0, sizeof(AppContext));
    strncpy(ctx->source_path, "input/sample.c", sizeof(ctx->source_path) - 1);
    ctx->config = formatter_default_config();
    formatter_diagnostics_init(&ctx->diagnostics);
}

static void free_app_context(AppContext *ctx) {
    if (ctx->original_code) {
        free(ctx->original_code);
        ctx->original_code = NULL;
    }
    if (ctx->formatted_code) {
        free(ctx->formatted_code);
        ctx->formatted_code = NULL;
    }
}

/* Menu Actions */
static void action_select_file(AppContext *ctx) {
    print_header("Select Source Code File");
    printf("Current file: %s%s%s\n", COLOR_CYAN, ctx->source_path[0] ? ctx->source_path : "(none)", COLOR_RESET);
    printf("Enter path to C source file (.c / .h): ");
    fflush(stdout);

    char input[512];
    if (!get_user_line(input, sizeof(input)) || strlen(str_trim(input)) == 0) {
        print_info("Keeping current file selection.");
        terminal_pause();
        return;
    }

    char *path = str_trim(input);
    if (!file_exists(path)) {
        print_error("File '%s' not found or is unreadable.", path);
        terminal_pause();
        return;
    }

    if (!validate_file_extension(path)) {
        print_warning("'%s' does not have a standard C extension (.c / .h).", path);
    }

    strncpy(ctx->source_path, path, sizeof(ctx->source_path) - 1);

    /* Reload original file into memory */
    if (ctx->original_code) {
        free(ctx->original_code);
        ctx->original_code = NULL;
    }
    if (ctx->formatted_code) {
        free(ctx->formatted_code);
        ctx->formatted_code = NULL;
        ctx->is_formatted = false;
    }

    char err_msg[256] = "";
    ctx->original_code = read_file_to_string(ctx->source_path, &ctx->original_size, err_msg, sizeof(err_msg));
    if (!ctx->original_code) {
        print_error("Error loading file: %s", err_msg);
    } else {
        print_success("File '%s' loaded successfully (%ld bytes).", ctx->source_path, ctx->original_size);
    }
    terminal_pause();
}

static void action_format_code(AppContext *ctx) {
    print_header("Format Source Code");
    if (!ctx->original_code) {
        if (file_exists(ctx->source_path)) {
            char err_msg[256] = "";
            ctx->original_code = read_file_to_string(ctx->source_path, &ctx->original_size, err_msg, sizeof(err_msg));
        }
        if (!ctx->original_code) {
            print_error("No source code loaded. Please select a valid file first (Option 1).");
            terminal_pause();
            return;
        }
    }

    if (ctx->formatted_code) {
        free(ctx->formatted_code);
        ctx->formatted_code = NULL;
    }

    formatter_diagnostics_init(&ctx->diagnostics);
    ctx->formatted_code = format_source_code(ctx->original_code, &ctx->config, &ctx->diagnostics);

    if (ctx->formatted_code) {
        ctx->is_formatted = true;
        print_success("Source code formatted successfully!");
        formatter_diagnostics_print(&ctx->diagnostics);
    } else {
        ctx->is_formatted = false;
        print_error("Failed to format source code.");
    }
    terminal_pause();
}

static void action_preview_original(AppContext *ctx) {
    print_header("Preview Original Source Code");
    if (!ctx->original_code) {
        if (file_exists(ctx->source_path)) {
            char err_msg[256] = "";
            ctx->original_code = read_file_to_string(ctx->source_path, &ctx->original_size, err_msg, sizeof(err_msg));
        }
        if (!ctx->original_code) {
            print_error("No source file loaded yet.");
            terminal_pause();
            return;
        }
    }
    display_code_with_line_numbers(ctx->original_code, "Original Source Code");
    terminal_pause();
}

static void action_preview_formatted(AppContext *ctx) {
    print_header("Preview Formatted Source Code");
    if (!ctx->is_formatted || !ctx->formatted_code) {
        print_warning("Code has not been formatted yet. Formatting now...");
        action_format_code(ctx);
        if (!ctx->formatted_code) return;
    }
    display_code_with_line_numbers(ctx->formatted_code, "Formatted Source Code (4-Space Indent, K&R Braces)");
    terminal_pause();
}

static void action_view_statistics(AppContext *ctx) {
    print_header("Source Code Statistics");
    if (!ctx->original_code) {
        if (file_exists(ctx->source_path)) {
            char err_msg[256] = "";
            ctx->original_code = read_file_to_string(ctx->source_path, &ctx->original_size, err_msg, sizeof(err_msg));
        }
        if (!ctx->original_code) {
            print_error("Please select and load a source file first.");
            terminal_pause();
            return;
        }
    }

    CodeStatistics orig_stats = calculate_code_statistics(ctx->original_code, ctx->original_size);

    if (ctx->is_formatted && ctx->formatted_code) {
        CodeStatistics fmt_stats = calculate_code_statistics(ctx->formatted_code, (long)strlen(ctx->formatted_code));
        print_statistics_comparison(&orig_stats, &fmt_stats, "Original", "Formatted");
    } else {
        print_code_statistics(&orig_stats, "Original Code Statistics");
        print_info("Format the code (Option 2) to view comparative before/after metrics.");
    }
    terminal_pause();
}

static void action_compare_codes(AppContext *ctx) {
    print_header("Compare Original vs Formatted Code");
    if (!ctx->original_code) {
        if (file_exists(ctx->source_path)) {
            char err_msg[256] = "";
            ctx->original_code = read_file_to_string(ctx->source_path, &ctx->original_size, err_msg, sizeof(err_msg));
        }
        if (!ctx->original_code) {
            print_error("No source code loaded.");
            terminal_pause();
            return;
        }
    }

    if (!ctx->is_formatted || !ctx->formatted_code) {
        print_info("Formatting code for comparison...");
        action_format_code(ctx);
        if (!ctx->formatted_code) return;
    }

    display_side_by_side_diff(ctx->original_code, ctx->formatted_code);
    terminal_pause();
}

static void action_settings(AppContext *ctx) {
    while (1) {
        terminal_clear();
        print_banner();
        printf("\n%s=== Formatter Settings ===%s\n", COLOR_CYAN, COLOR_RESET);
        printf("  1. Indentation Width      : %s%d spaces%s\n", COLOR_BOLD, ctx->config.indent_size, COLOR_RESET);
        printf("  2. Indentation Type       : %s%s%s\n", COLOR_BOLD, ctx->config.use_tabs ? "Tabs (\\t)" : "Spaces", COLOR_RESET);
        printf("  3. Space Around Operators : %s%s%s\n", COLOR_BOLD, ctx->config.space_around_binary ? "Enabled" : "Disabled", COLOR_RESET);
        printf("  4. Space After Commas     : %s%s%s\n", COLOR_BOLD, ctx->config.space_after_comma ? "Enabled" : "Disabled", COLOR_RESET);
        printf("  5. Space After Keywords   : %s%s%s\n", COLOR_BOLD, ctx->config.space_after_keyword ? "Enabled" : "Disabled", COLOR_RESET);
        printf("  6. Brace Style            : %s%s%s\n", COLOR_BOLD, ctx->config.knr_braces ? "K&R (same line)" : "Allman (new line)", COLOR_RESET);
        printf("  7. Strip Trailing Whitespace : %s%s%s\n", COLOR_BOLD, ctx->config.remove_trailing_ws ? "Enabled" : "Disabled", COLOR_RESET);
        printf("  8. Reset to Defaults\n");
        printf("  9. Back to Main Menu\n");
        print_separator('-', 40);

        int choice = get_user_choice(1, 9);
        if (choice == 1) {
            printf("Enter indent spaces (2, 4, 8): ");
            int sp = get_user_choice(1, 16);
            if (sp > 0) ctx->config.indent_size = sp;
            ctx->is_formatted = false;
        } else if (choice == 2) {
            ctx->config.use_tabs = !ctx->config.use_tabs;
            ctx->is_formatted = false;
        } else if (choice == 3) {
            ctx->config.space_around_binary = !ctx->config.space_around_binary;
            ctx->is_formatted = false;
        } else if (choice == 4) {
            ctx->config.space_after_comma = !ctx->config.space_after_comma;
            ctx->is_formatted = false;
        } else if (choice == 5) {
            ctx->config.space_after_keyword = !ctx->config.space_after_keyword;
            ctx->is_formatted = false;
        } else if (choice == 6) {
            ctx->config.knr_braces = !ctx->config.knr_braces;
            ctx->is_formatted = false;
        } else if (choice == 7) {
            ctx->config.remove_trailing_ws = !ctx->config.remove_trailing_ws;
            ctx->is_formatted = false;
        } else if (choice == 8) {
            ctx->config = formatter_default_config();
            ctx->is_formatted = false;
            print_success("Settings reset to defaults.");
            terminal_pause();
        } else if (choice == 9 || choice == -1) {
            break;
        }
    }
}

static void action_save_file(AppContext *ctx) {
    print_header("Save Formatted File");
    if (!ctx->is_formatted || !ctx->formatted_code) {
        print_warning("No formatted code available to save. Please format first.");
        terminal_pause();
        return;
    }

    printf("Default output path: %s%s%s\n", COLOR_CYAN, DEFAULT_OUTPUT_PATH, COLOR_RESET);
    printf("Enter output path (press Enter for default): ");
    fflush(stdout);

    char out_path[512] = "";
    if (!get_user_line(out_path, sizeof(out_path)) || strlen(str_trim(out_path)) == 0) {
        strncpy(out_path, DEFAULT_OUTPUT_PATH, sizeof(out_path) - 1);
    } else {
        strncpy(out_path, str_trim(out_path), sizeof(out_path) - 1);
    }

    /* Protection: never overwrite original file without confirmation */
    if (strcmp(out_path, ctx->source_path) == 0 || file_exists(out_path)) {
        printf("%sWarning: Target file '%s' already exists!%s\n", COLOR_YELLOW, out_path, COLOR_RESET);
        printf("Do you want to overwrite it? (y/n) [n]: ");
        fflush(stdout);
        char confirm[16] = "";
        get_user_line(confirm, sizeof(confirm));
        if (confirm[0] != 'y' && confirm[0] != 'Y') {
            print_info("Save operation cancelled.");
            terminal_pause();
            return;
        }

        /* Safety backup creation */
        printf("Create backup file (%s.bak) before overwriting? (y/n) [y]: ", out_path);
        fflush(stdout);
        char backup_confirm[16] = "";
        get_user_line(backup_confirm, sizeof(backup_confirm));
        if (backup_confirm[0] != 'n' && backup_confirm[0] != 'N') {
            char backup_path[512] = "";
            char err_backup[256] = "";
            if (create_backup_file(out_path, backup_path, sizeof(backup_path), err_backup, sizeof(err_backup))) {
                print_success("Backup created at: %s", backup_path);
            } else {
                print_warning("Could not create backup: %s", err_backup);
            }
        }
    }

    char err_msg[256] = "";
    if (write_string_to_file(out_path, ctx->formatted_code, err_msg, sizeof(err_msg))) {
        print_success("Formatted file successfully saved to '%s' (%zu bytes).", out_path, strlen(ctx->formatted_code));
    } else {
        print_error("Failed to write to '%s': %s", out_path, err_msg);
    }
    terminal_pause();
}

static void action_batch_format(AppContext *ctx) {
    print_header("Batch Format Multiple C Files");
    printf("Enter paths to C files separated by space or comma:\n> ");
    fflush(stdout);

    char input[1024];
    if (!get_user_line(input, sizeof(input)) || strlen(str_trim(input)) == 0) {
        print_info("No files entered. Returning to menu.");
        terminal_pause();
        return;
    }

    char *files[32];
    int count = 0;
    char *token = strtok(input, " ,;");
    while (token && count < 32) {
        files[count++] = safe_strdup(str_trim(token));
        token = strtok(NULL, " ,;");
    }

    if (count > 0) {
        batch_format_files((const char **)files, count, &ctx->config, "output");
        for (int i = 0; i < count; ++i) {
            free(files[i]);
        }
    } else {
        print_warning("No valid files parsed.");
    }
    terminal_pause();
}

/* External test runner prototype */
int run_formatter_tests(void);

static void action_run_tests(void) {
    print_header("Automated Formatter Test Suite");
    printf("Executing built-in unit and integration tests...\n\n");
    int failures = run_formatter_tests();
    if (failures == 0) {
        print_success("All automated test cases passed successfully!");
    } else {
        print_error("%d test case(s) failed.", failures);
    }
    terminal_pause();
}

/* CLI Argument Processing */
static void print_cli_help(const char *prog_name) {
    printf("%sMini Source Code Formatter v%s (C Language)%s\n", COLOR_CYAN, VERSION, COLOR_RESET);
    printf("Usage:\n");
    printf("  %s [options] <input_file>\n", prog_name);
    printf("  %s (without arguments for interactive menu)\n\n", prog_name);
    printf("Options:\n");
    printf("  -o <path>       Specify output file path (default: %s)\n", DEFAULT_OUTPUT_PATH);
    printf("  -i, --indent <N> Set indentation size (spaces, default: 4)\n");
    printf("  -t, --tabs      Use tab characters instead of spaces\n");
    printf("  --preview       Print formatted code to standard output\n");
    printf("  --diff          Display side-by-side comparison of original vs formatted\n");
    printf("  --stats         Display code metrics and before/after statistics\n");
    printf("  --backup        Create a backup file if output file exists\n");
    printf("  --batch <files> Batch format multiple C files into output/ directory\n");
    printf("  --test          Run automated unit test suite and exit\n");
    printf("  -h, --help      Display this help message\n");
    printf("  -v, --version   Display version information\n");
}

int main(int argc, char *argv[]) {
    AppContext ctx;
    init_app_context(&ctx);

    /* CLI Mode */
    if (argc > 1) {
        const char *input_path = NULL;
        const char *output_path = NULL;
        bool preview_flag = false;
        bool diff_flag = false;
        bool stats_flag = false;
        bool backup_flag = false;
        bool batch_mode = false;
        int batch_start_index = 0;

        for (int i = 1; i < argc; ++i) {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                print_cli_help(argv[0]);
                free_app_context(&ctx);
                return EXIT_SUCCESS;
            } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
                printf("Mini Source Code Formatter v%s (C11 standard)\n", VERSION);
                free_app_context(&ctx);
                return EXIT_SUCCESS;
            } else if (strcmp(argv[i], "--test") == 0) {
                int res = run_formatter_tests();
                free_app_context(&ctx);
                return (res == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
            } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
                output_path = argv[++i];
            } else if ((strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--indent") == 0) && i + 1 < argc) {
                ctx.config.indent_size = atoi(argv[++i]);
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tabs") == 0) {
                ctx.config.use_tabs = true;
            } else if (strcmp(argv[i], "--preview") == 0) {
                preview_flag = true;
            } else if (strcmp(argv[i], "--diff") == 0) {
                diff_flag = true;
            } else if (strcmp(argv[i], "--stats") == 0) {
                stats_flag = true;
            } else if (strcmp(argv[i], "--backup") == 0) {
                backup_flag = true;
            } else if (strcmp(argv[i], "--batch") == 0) {
                batch_mode = true;
                batch_start_index = i + 1;
                break;
            } else if (argv[i][0] != '-') {
                input_path = argv[i];
            }
        }

        if (batch_mode) {
            int count = argc - batch_start_index;
            if (count <= 0) {
                print_error("No files specified for batch formatting.");
                free_app_context(&ctx);
                return EXIT_FAILURE;
            }
            batch_format_files((const char **)&argv[batch_start_index], count, &ctx.config, "output");
            free_app_context(&ctx);
            return EXIT_SUCCESS;
        }

        if (!input_path) {
            print_error("No input file specified. Run '%s --help' for usage.", argv[0]);
            free_app_context(&ctx);
            return EXIT_FAILURE;
        }

        /* Process single file CLI */
        char err_msg[256] = "";
        long size = 0;
        char *source = read_file_to_string(input_path, &size, err_msg, sizeof(err_msg));
        if (!source) {
            print_error("Failed to read input file '%s': %s", input_path, err_msg);
            free_app_context(&ctx);
            return EXIT_FAILURE;
        }

        FormatterDiagnostics diag;
        char *formatted = format_source_code(source, &ctx.config, &diag);
        if (!formatted) {
            print_error("Formatting failed for '%s'.", input_path);
            free(source);
            free_app_context(&ctx);
            return EXIT_FAILURE;
        }

        if (diag.warning_count > 0) {
            formatter_diagnostics_print(&diag);
        }

        if (stats_flag) {
            CodeStatistics orig_s = calculate_code_statistics(source, size);
            CodeStatistics fmt_s = calculate_code_statistics(formatted, (long)strlen(formatted));
            print_statistics_comparison(&orig_s, &fmt_s, input_path, output_path ? output_path : "Formatted");
        }

        if (diff_flag) {
            display_side_by_side_diff(source, formatted);
        }

        if (preview_flag) {
            display_code_with_line_numbers(formatted, "Formatted Code Preview");
        }

        const char *target_out = output_path ? output_path : DEFAULT_OUTPUT_PATH;
        if (backup_flag && file_exists(target_out)) {
            char bak_p[512];
            create_backup_file(target_out, bak_p, sizeof(bak_p), NULL, 0);
        }

        if (!write_string_to_file(target_out, formatted, err_msg, sizeof(err_msg))) {
            print_error("Failed to save output to '%s': %s", target_out, err_msg);
            free(source);
            free(formatted);
            free_app_context(&ctx);
            return EXIT_FAILURE;
        }

        print_success("Formatted file successfully written to '%s'", target_out);
        free(source);
        free(formatted);
        free_app_context(&ctx);
        return EXIT_SUCCESS;
    }

    /* Interactive Menu Mode */
    if (file_exists(ctx.source_path)) {
        char err[256];
        ctx.original_code = read_file_to_string(ctx.source_path, &ctx.original_size, err, sizeof(err));
    }

    while (1) {
        terminal_clear();
        print_banner();
        printf("Current File: %s%s%s | Status: %s%s%s\n",
               COLOR_CYAN, ctx.source_path[0] ? ctx.source_path : "(none)", COLOR_RESET,
               ctx.is_formatted ? COLOR_GREEN : COLOR_YELLOW,
               ctx.is_formatted ? "Formatted" : "Unformatted", COLOR_RESET);
        print_separator('-', 50);
        printf("  1. Select Source Code File\n");
        printf("  2. Format Source Code\n");
        printf("  3. Preview Original Code\n");
        printf("  4. Preview Formatted Code\n");
        printf("  5. View Code Statistics\n");
        printf("  6. Compare Original and Formatted\n");
        printf("  7. Formatter Settings\n");
        printf("  8. Save Formatted File\n");
        printf("  9. Batch Format Multiple Files\n");
        printf(" 10. Run Automated Tests\n");
        printf("  0. Exit\n");
        print_separator('=', 50);

        int choice = get_user_choice(0, 10);
        if (choice == -1 || choice == 0) {
            print_info("Exiting Mini Source Code Formatter. Goodbye!");
            break;
        }

        switch (choice) {
            case 1:  action_select_file(&ctx); break;
            case 2:  action_format_code(&ctx); break;
            case 3:  action_preview_original(&ctx); break;
            case 4:  action_preview_formatted(&ctx); break;
            case 5:  action_view_statistics(&ctx); break;
            case 6:  action_compare_codes(&ctx); break;
            case 7:  action_settings(&ctx); break;
            case 8:  action_save_file(&ctx); break;
            case 9:  action_batch_format(&ctx); break;
            case 10: action_run_tests(); break;
            default: break;
        }
    }

    free_app_context(&ctx);
    return EXIT_SUCCESS;
}
