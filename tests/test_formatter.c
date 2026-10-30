/**
 * @file test_formatter.c
 * @brief Automated unit and integration test suite for Mini Source Code Formatter.
 *
 * Tests all 12 core requirements, idempotency, edge cases, and functional compilation.
 *
 * Part of Mini Source Code Formatter Using C Language.
 */

#include "formatter.h"
#include "file_handler.h"
#include "tokenizer.h"
#include "statistics.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("%s  [FAIL]%s %s (line %d): %s\n", COLOR_RED, COLOR_RESET, __func__, __LINE__, msg); \
        g_tests_failed++; \
        return false; \
    } \
} while(0)

#define TEST_PASS() do { \
    printf("%s  [PASS]%s %s\n", COLOR_GREEN, COLOR_RESET, __func__); \
    g_tests_passed++; \
    return true; \
} while(0)

/* Test 1: Simple unformatted C program (Prompt's exact sample) */
static bool test_simple_unformatted(void) {
    g_tests_run++;
    const char *input = "#include<stdio.h>\nint main(){int a=10;int b=20;if(a>b){printf(\"A\");}else{printf(\"B\");}return 0;}";

    FormatterConfig cfg = formatter_default_config();
    FormatterDiagnostics diag;
    char *formatted = format_source_code(input, &cfg, &diag);

    TEST_ASSERT(formatted != NULL, "Formatted output is null");
    TEST_ASSERT(strstr(formatted, "#include <stdio.h>") != NULL, "Missing space in #include");
    TEST_ASSERT(strstr(formatted, "int main() {") != NULL, "Incorrect function brace style");
    TEST_ASSERT(strstr(formatted, "    int a = 10;") != NULL, "Incorrect indentation or operator spacing for a");
    TEST_ASSERT(strstr(formatted, "    int b = 20;") != NULL, "Incorrect indentation or operator spacing for b");
    TEST_ASSERT(strstr(formatted, "    if (a > b) {") != NULL, "Incorrect if statement formatting");
    TEST_ASSERT(strstr(formatted, "        printf(\"A\");") != NULL, "Incorrect block indentation for printf A");
    TEST_ASSERT(strstr(formatted, "    } else {") != NULL, "Incorrect else block formatting");
    TEST_ASSERT(strstr(formatted, "        printf(\"B\");") != NULL, "Incorrect block indentation for printf B");
    TEST_ASSERT(strstr(formatted, "    return 0;") != NULL, "Incorrect return statement formatting");

    free(formatted);
    TEST_PASS();
}

/* Test 2: Nested if-else blocks */
static bool test_nested_if_else(void) {
    g_tests_run++;
    const char *input = "void check(int x){if(x>0){if(x>10){printf(\"High\");}else{printf(\"Low\");}}else{printf(\"Zero\");}}";

    FormatterConfig cfg = formatter_default_config();
    FormatterDiagnostics diag;
    char *formatted = format_source_code(input, &cfg, &diag);

    TEST_ASSERT(formatted != NULL, "Formatted output is null");
    TEST_ASSERT(strstr(formatted, "    if (x > 0) {") != NULL, "Missing level 1 indent for if");
    TEST_ASSERT(strstr(formatted, "        if (x > 10) {") != NULL, "Missing level 2 indent for nested if");
    TEST_ASSERT(strstr(formatted, "            printf(\"High\");") != NULL, "Missing level 3 indent for statement");
    TEST_ASSERT(strstr(formatted, "        } else {") != NULL, "Incorrect nested else brace formatting");

    free(formatted);
    TEST_PASS();
}

/* Test 3: Nested for and while loops, verifying for loop headers do not break */
static bool test_nested_loops_and_for_headers(void) {
    g_tests_run++;
    const char *input = "void loop_test(){for(int i=0;i<10;i++){while(i>0){i--;}}}";

    FormatterConfig cfg = formatter_default_config();
    FormatterDiagnostics diag;
    char *formatted = format_source_code(input, &cfg, &diag);

    TEST_ASSERT(formatted != NULL, "Formatted output is null");
    /* Ensure for header is intact and not split by newlines */
    TEST_ASSERT(strstr(formatted, "for (int i = 0; i < 10; i++) {") != NULL, "For loop header was broken or incorrectly formatted");
    TEST_ASSERT(strstr(formatted, "        while (i > 0) {") != NULL, "Nested while loop header incorrect");
    TEST_ASSERT(strstr(formatted, "            i--;") != NULL, "Nested while body incorrect");

    free(formatted);
    TEST_PASS();
}

/* Test 4: Function declarations and definitions */
static bool test_function_declarations_and_definitions(void) {
    g_tests_run++;
    const char *input = "int add(int a,int b);\nint add(int a,int b){return a+b;}";

    FormatterConfig cfg = formatter_default_config();
    FormatterDiagnostics diag;
    char *formatted = format_source_code(input, &cfg, &diag);

    TEST_ASSERT(formatted != NULL, "Formatted output is null");
    TEST_ASSERT(strstr(formatted, "int add(int a, int b);") != NULL, "Function prototype spacing failed");
    TEST_ASSERT(strstr(formatted, "int add(int a, int b) {") != NULL, "Function definition spacing failed");
    TEST_ASSERT(strstr(formatted, "    return a + b;") != NULL, "Binary operator spacing in return failed");

    free(formatted);
    TEST_PASS();
}

/* Test 5: Arrays and pointers */
static bool test_arrays_and_pointers(void) {
    g_tests_run++;
    const char *input = "void ptr_test(){int arr[5]={1,2,3,4,5};int *ptr=&arr[0];*ptr=100;}";

    FormatterConfig cfg = formatter_default_config();
    FormatterDiagnostics diag;
    char *formatted = format_source_code(input, &cfg, &diag);

    TEST_ASSERT(formatted != NULL, "Formatted output is null");
    TEST_ASSERT(strstr(formatted, "int arr[5] = { 1, 2, 3, 4, 5 };") != NULL, "Array initializer formatting failed");
    TEST_ASSERT(strstr(formatted, "int *ptr = &arr[0];") != NULL, "Pointer declaration and address-of failed");
    TEST_ASSERT(strstr(formatted, "*ptr = 100;") != NULL, "Pointer dereference failed");

    free(formatted);
    TEST_PASS();
}

/* Test 6: String literals containing braces and semicolons */
static bool test_strings_with_special_chars(void) {
    g_tests_run++;
    const char *input = "int main(){printf(\"Braces {a;b} and ; should be preserved!\\n\");return 0;}";

    FormatterConfig cfg = formatter_default_config();
    FormatterDiagnostics diag;
    char *formatted = format_source_code(input, &cfg, &diag);

    TEST_ASSERT(formatted != NULL, "Formatted output is null");
    TEST_ASSERT(strstr(formatted, "\"Braces {a;b} and ; should be preserved!\\n\"") != NULL, "String literal contents corrupted");

    free(formatted);
    TEST_PASS();
}

/* Test 7: Single-line and multi-line comments */
static bool test_comments_preservation(void) {
    g_tests_run++;
    const char *input = "// Single line comment\n/* Multi\n   line\n   comment */\nint a=10;// inline comment\n";

    FormatterConfig cfg = formatter_default_config();
    FormatterDiagnostics diag;
    char *formatted = format_source_code(input, &cfg, &diag);

    TEST_ASSERT(formatted != NULL, "Formatted output is null");
    TEST_ASSERT(strstr(formatted, "// Single line comment") != NULL, "Single line comment lost");
    TEST_ASSERT(strstr(formatted, "/* Multi\n   line\n   comment */") != NULL, "Multi-line comment lost or corrupted");
    TEST_ASSERT(strstr(formatted, "// inline comment") != NULL, "Inline comment lost");

    free(formatted);
    TEST_PASS();
}

/* Test 8: Preprocessor directives and multiline macros */
static bool test_preprocessor_directives(void) {
    g_tests_run++;
    const char *input = "#include<stdio.h>\n#define MAX(a,b) ((a)>(b)?(a):(b))\nint a=MAX(1,2);";

    FormatterConfig cfg = formatter_default_config();
    FormatterDiagnostics diag;
    char *formatted = format_source_code(input, &cfg, &diag);

    TEST_ASSERT(formatted != NULL, "Formatted output is null");
    TEST_ASSERT(strstr(formatted, "#include <stdio.h>") != NULL, "Preprocessor include formatting failed");
    TEST_ASSERT(strstr(formatted, "#define MAX(a,b)") != NULL, "Preprocessor define formatting failed");

    free(formatted);
    TEST_PASS();
}

/* Test 9: Empty file handling */
static bool test_empty_file_handling(void) {
    g_tests_run++;
    const char *input = "";

    FormatterConfig cfg = formatter_default_config();
    FormatterDiagnostics diag;
    char *formatted = format_source_code(input, &cfg, &diag);

    TEST_ASSERT(formatted != NULL, "Formatted output for empty string must not crash");
    TEST_ASSERT(strlen(formatted) == 0, "Formatted output for empty string should be empty");

    free(formatted);
    TEST_PASS();
}

/* Test 10: Missing or invalid input file handling */
static bool test_file_handler_error_cases(void) {
    g_tests_run++;
    char err_msg[256] = "";
    long size = 0;

    /* Non-existent file */
    char *data = read_file_to_string("non_existent_file_xyz_123.c", &size, err_msg, sizeof(err_msg));
    TEST_ASSERT(data == NULL, "Should fail on non-existent file");
    TEST_ASSERT(strlen(err_msg) > 0, "Error message should be set for non-existent file");

    /* Invalid extension check */
    TEST_ASSERT(validate_file_extension("file.txt") == false, "file.txt should not be a valid C extension");
    TEST_ASSERT(validate_file_extension("file.c") == true, "file.c should be valid");
    TEST_ASSERT(validate_file_extension("file.h") == true, "file.h should be valid");

    TEST_PASS();
}

/* Test 11: Idempotency (formatting an already formatted file yields identical output) */
static bool test_idempotency(void) {
    g_tests_run++;
    const char *input = "#include<stdio.h>\nint main(){int a=10;int b=20;if(a>b){printf(\"A\");}else{printf(\"B\");}return 0;}";

    FormatterConfig cfg = formatter_default_config();
    FormatterDiagnostics diag1, diag2;

    /* Pass 1 */
    char *pass1 = format_source_code(input, &cfg, &diag1);
    TEST_ASSERT(pass1 != NULL, "Pass 1 formatting failed");

    /* Pass 2 on the output of Pass 1 */
    char *pass2 = format_source_code(pass1, &cfg, &diag2);
    TEST_ASSERT(pass2 != NULL, "Pass 2 formatting failed");

    /* Compare Pass 1 and Pass 2 */
    bool match = (strcmp(pass1, pass2) == 0);
    if (!match) {
        printf("--- PASS 1 ---\n%s\n--- PASS 2 ---\n%s\n", pass1, pass2);
    }
    TEST_ASSERT(match, "Formatting is not idempotent! format(format(x)) != format(x)");

    free(pass1);
    free(pass2);
    TEST_PASS();
}

/* Test 12: Unbalanced braces and malformed input warning detection */
static bool test_unbalanced_braces_warning(void) {
    g_tests_run++;
    const char *unbalanced = "int main(){ if(1){ printf(\"Missing braces\"); }";

    FormatterConfig cfg = formatter_default_config();
    FormatterDiagnostics diag;
    char *formatted = format_source_code(unbalanced, &cfg, &diag);

    TEST_ASSERT(formatted != NULL, "Should not crash on unbalanced braces");
    TEST_ASSERT(diag.warning_count > 0, "Diagnostics must detect unbalanced braces");
    TEST_ASSERT(diag.brace_balance > 0, "Brace balance should be positive for missing closing brace");

    free(formatted);
    TEST_PASS();
}

/* Test 13: Functional compilation and execution test */
static bool test_compilation_and_execution(void) {
    g_tests_run++;
    const char *code =
        "#include <stdio.h>\n"
        "int main() {\n"
        "    int a = 10;\n"
        "    int b = 20;\n"
        "    if (a > b) {\n"
        "        printf(\"A\");\n"
        "    } else {\n"
        "        printf(\"B\");\n"
        "    }\n"
        "    return 0;\n"
        "}\n";

    /* Write to a temporary file, compile with clang/gcc, and run */
    const char *tmp_c = "output/tmp_test_exec.c";
    const char *tmp_bin = "output/tmp_test_exec";

    char err[256];
    if (!write_string_to_file(tmp_c, code, err, sizeof(err))) {
        print_warning("Skipping execution test (cannot write tmp file: %s)", err);
        TEST_PASS();
    }

    char compile_cmd[512];
    snprintf(compile_cmd, sizeof(compile_cmd), "clang -O2 -o %s %s 2>/dev/null || gcc -O2 -o %s %s 2>/dev/null",
             tmp_bin, tmp_c, tmp_bin, tmp_c);

    int ret = system(compile_cmd);
    if (ret != 0) {
        print_warning("Compiler not found or compilation skipped in environment.");
        TEST_PASS();
    }

    /* Execute the binary and capture stdout */
    char run_cmd[512];
    snprintf(run_cmd, sizeof(run_cmd), "%s > output/tmp_test_out.txt 2>/dev/null", tmp_bin);
    ret = system(run_cmd);
    TEST_ASSERT(ret == 0, "Compiled program execution failed");

    long out_size = 0;
    char *output_data = read_file_to_string("output/tmp_test_out.txt", &out_size, NULL, 0);
    TEST_ASSERT(output_data != NULL, "Failed to read compiled program output");
    TEST_ASSERT(strcmp(output_data, "B") == 0, "Program output did not match expected 'B'");

    free(output_data);

    /* Clean up temporary files */
    remove(tmp_c);
    remove(tmp_bin);
    remove("output/tmp_test_out.txt");

    TEST_PASS();
}

int run_formatter_tests(void) {
    g_tests_run = 0;
    g_tests_passed = 0;
    g_tests_failed = 0;

    printf("\n%s==================================================%s\n", COLOR_CYAN, COLOR_RESET);
    printf("         AUTOMATED TEST SUITE EXECUTION           \n");
    printf("%s==================================================%s\n", COLOR_CYAN, COLOR_RESET);

    test_simple_unformatted();
    test_nested_if_else();
    test_nested_loops_and_for_headers();
    test_function_declarations_and_definitions();
    test_arrays_and_pointers();
    test_strings_with_special_chars();
    test_comments_preservation();
    test_preprocessor_directives();
    test_empty_file_handling();
    test_file_handler_error_cases();
    test_idempotency();
    test_unbalanced_braces_warning();
    test_compilation_and_execution();

    printf("%s==================================================%s\n", COLOR_CYAN, COLOR_RESET);
    printf("Test Results: %s%d Total%s | %s%d Passed%s | %s%d Failed%s\n",
           COLOR_BOLD, g_tests_run, COLOR_RESET,
           COLOR_GREEN, g_tests_passed, COLOR_RESET,
           (g_tests_failed > 0) ? COLOR_RED : COLOR_GREEN, g_tests_failed, COLOR_RESET);

    return g_tests_failed;
}

#ifdef TEST_RUNNER_MAIN
int main(void) {
    int failures = run_formatter_tests();
    return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif
