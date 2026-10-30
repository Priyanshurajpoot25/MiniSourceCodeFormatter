/**
 * @file formatter.c
 * @brief Implementation of the C code formatting engine.
 *
 * Part of Mini Source Code Formatter Using C Language.
 */

#include "formatter.h"
#include "tokenizer.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

FormatterConfig formatter_default_config(void) {
    FormatterConfig config;
    config.indent_size = 4;
    config.use_tabs = false;
    config.space_around_binary = true;
    config.space_after_comma = true;
    config.space_after_keyword = true;
    config.knr_braces = true;
    config.remove_trailing_ws = true;
    config.max_blank_lines = 1;
    return config;
}

void formatter_diagnostics_init(FormatterDiagnostics *diag) {
    if (!diag) return;
    diag->brace_balance = 0;
    diag->paren_balance = 0;
    diag->square_balance = 0;
    diag->warning_count = 0;
    for (int i = 0; i < MAX_WARNINGS; ++i) {
        diag->warnings[i][0] = '\0';
    }
}

void formatter_diagnostics_add_warning(FormatterDiagnostics *diag, const char *fmt, ...) {
    if (!diag || diag->warning_count >= MAX_WARNINGS) return;
    va_list args;
    va_start(args, fmt);
    vsnprintf(diag->warnings[diag->warning_count], MAX_WARNING_LEN, fmt, args);
    va_end(args);
    diag->warning_count++;
}

void formatter_diagnostics_print(const FormatterDiagnostics *diag) {
    if (!diag) return;
    if (diag->warning_count == 0) {
        print_success("Diagnostics: No syntax or balance warnings detected.");
        return;
    }
    printf("\n%sDiagnostics Warnings (%d found):%s\n", COLOR_YELLOW, diag->warning_count, COLOR_RESET);
    for (int i = 0; i < diag->warning_count; ++i) {
        printf("  %s[%d]%s %s\n", COLOR_YELLOW, i + 1, COLOR_RESET, diag->warnings[i]);
    }
}

/* Internal formatting helper state */
typedef struct {
    StringBuilder *sb;
    const FormatterConfig *config;
    FormatterDiagnostics *diag;
    int brace_balance;
    int indent_level;
    bool at_line_start;
    int paren_depth;
    int square_depth;
    int for_paren_depth;
    bool awaiting_for_paren;
    int ternary_depth;
    int consecutive_blank_lines;
    bool in_case;
    bool prev_was_do;
    bool in_initializer;
} FormatState;

static void emit_indent(FormatState *st) {
    if (st->at_line_start) {
        int level = st->indent_level;
        /* Labels and case/default: indent 1 level less if inside a block */
        if (st->in_case && level > 0) {
            level = level - 1;
        }
        sb_append_indent(st->sb, level, st->config->indent_size, st->config->use_tabs);
        st->at_line_start = false;
        st->consecutive_blank_lines = 0;
    }
}

static void emit_newline(FormatState *st) {
    if (st->config->remove_trailing_ws) {
        sb_trim_trailing_whitespace(st->sb);
    }
    sb_append_char(st->sb, '\n');
    st->at_line_start = true;
    st->consecutive_blank_lines = 0;
}

static void emit_blank_line(FormatState *st) {
    if (!st->at_line_start) {
        emit_newline(st);
    }
    if (st->consecutive_blank_lines < st->config->max_blank_lines) {
        sb_append_char(st->sb, '\n');
        st->consecutive_blank_lines++;
        st->at_line_start = true;
    }
}

static void ensure_space_before(FormatState *st) {
    if (st->at_line_start) {
        emit_indent(st);
    } else {
        char last = sb_last_char(st->sb);
        if (last != ' ' && last != '\t' && last != '\n' && last != '\0' && last != '(' && last != '[') {
            sb_append_char(st->sb, ' ');
        }
    }
}

static void normalize_preprocessor(FormatState *st, const char *raw_dir) {
    /* If directive is #include<stdio.h>, format as #include <stdio.h> */
    const char *p = raw_dir;
    while (*p && isspace((unsigned char)*p)) p++;

    if (strncmp(p, "#include", 8) == 0) {
        const char *after = p + 8;
        if (*after != ' ' && *after != '\t') {
            sb_append_str(st->sb, "#include ");
            sb_append_str(st->sb, after);
            return;
        }
    } else if (strncmp(p, "#define", 7) == 0) {
        const char *after = p + 7;
        if (*after != ' ' && *after != '\t') {
            sb_append_str(st->sb, "#define ");
            sb_append_str(st->sb, after);
            return;
        }
    }
    sb_append_str(st->sb, p);
}

char *format_tokens(const TokenList *list, const FormatterConfig *config, FormatterDiagnostics *diag) {
    if (!list) return NULL;

    FormatterConfig cfg = config ? *config : formatter_default_config();
    if (diag) formatter_diagnostics_init(diag);

    StringBuilder sb;
    sb_init(&sb);

    FormatState st;
    st.sb = &sb;
    st.config = &cfg;
    st.diag = diag;
    st.brace_balance = 0;
    st.indent_level = 0;
    st.at_line_start = true;
    st.paren_depth = 0;
    st.square_depth = 0;
    st.for_paren_depth = 0;
    st.awaiting_for_paren = false;
    st.ternary_depth = 0;
    st.consecutive_blank_lines = 0;
    st.in_case = false;
    st.prev_was_do = false;
    st.in_initializer = false;

    for (size_t i = 0; i < list->count; ++i) {
        const Token *tok = &list->tokens[i];
        const Token *prev = (i > 0) ? &list->tokens[i - 1] : NULL;
        const Token *next = (i + 1 < list->count) ? &list->tokens[i + 1] : NULL;

        if (tok->type == TOKEN_EOF) {
            break;
        }

        /* 1. Preprocessor Directives */
        if (tok->type == TOKEN_PREPROCESSOR) {
            if (!st.at_line_start) {
                emit_newline(&st);
            }
            normalize_preprocessor(&st, tok->value);
            emit_newline(&st);

            /* If the following token is not preprocessor, add a blank line for readability */
            if (next && next->type != TOKEN_PREPROCESSOR && next->type != TOKEN_EOF) {
                emit_blank_line(&st);
            }
            continue;
        }

        /* 2. Comments */
        if (tok->type == TOKEN_COMMENT_SINGLE) {
            if (st.at_line_start) {
                emit_indent(&st);
                sb_append_str(&sb, tok->value);
            } else {
                sb_append_str(&sb, "  ");
                sb_append_str(&sb, tok->value);
            }
            emit_newline(&st);
            continue;
        }

        if (tok->type == TOKEN_COMMENT_MULTI) {
            if (st.at_line_start) {
                emit_indent(&st);
                sb_append_str(&sb, tok->value);
                emit_newline(&st);
            } else {
                sb_append_char(&sb, ' ');
                sb_append_str(&sb, tok->value);
                if (next && next->has_leading_newline) {
                    emit_newline(&st);
                }
            }
            continue;
        }

        /* 3. Opening Brace { */
        if (tok->type == TOKEN_PUNCTUATION && strcmp(tok->value, "{") == 0) {
            st.brace_balance++;
            if (diag) diag->brace_balance++;

            /* Check if this brace is an array/struct initializer */
            bool is_initializer = false;
            if (prev && (token_is_assignment_operator(prev) || strcmp(prev->value, "[") == 0 || st.in_initializer)) {
                is_initializer = true;
                st.in_initializer = true;
            }

            if (is_initializer) {
                ensure_space_before(&st);
                sb_append_char(&sb, '{');
                sb_append_char(&sb, ' ');
            } else {
                if (st.config->knr_braces) {
                    if (!st.at_line_start) {
                        ensure_space_before(&st);
                    } else {
                        emit_indent(&st);
                    }
                    sb_append_char(&sb, '{');
                } else {
                    /* Allman style: brace on new line */
                    if (!st.at_line_start) {
                        emit_newline(&st);
                    }
                    emit_indent(&st);
                    sb_append_char(&sb, '{');
                }
                st.indent_level++;
                emit_newline(&st);
            }
            continue;
        }

        /* 4. Closing Brace } */
        if (tok->type == TOKEN_PUNCTUATION && strcmp(tok->value, "}") == 0) {
            st.brace_balance--;
            if (diag) diag->brace_balance--;

            if (st.in_initializer) {
                /* Array/struct initializer closing brace */
                char last = sb_last_char(&sb);
                if (last != ' ' && last != '{') {
                    sb_append_char(&sb, ' ');
                }
                sb_append_char(&sb, '}');
                st.in_initializer = false;
                continue;
            }

            st.indent_level--;
            if (st.indent_level < 0) {
                if (diag) {
                    formatter_diagnostics_add_warning(diag, "Line %zu: Extra closing brace '}' without matching opening brace.", tok->line);
                }
                st.indent_level = 0;
            }

            if (!st.at_line_start) {
                emit_newline(&st);
            }
            emit_indent(&st);
            sb_append_char(&sb, '}');

            /* Check next token for K&R continuation: } else ... or } while ... */
            if (next && next->type == TOKEN_KEYWORD && strcmp(next->value, "else") == 0) {
                sb_append_char(&sb, ' ');
                /* 'else' will follow on the same line */
            } else if (next && next->type == TOKEN_KEYWORD && strcmp(next->value, "while") == 0 && st.prev_was_do) {
                sb_append_char(&sb, ' ');
                st.prev_was_do = false;
            } else if (next && next->type == TOKEN_PUNCTUATION && (strcmp(next->value, ";") == 0 || strcmp(next->value, ",") == 0)) {
                /* struct Foo { ... }; or typedef enum { ... } Name; */
                /* Do not emit newline yet; semicolon or comma follows directly */
            } else {
                emit_newline(&st);
                /* If closing top-level function or block before return, add blank line */
                if (st.indent_level == 0) {
                    if (next && next->type != TOKEN_EOF) {
                        emit_blank_line(&st);
                    }
                } else if (next && next->type == TOKEN_KEYWORD && strcmp(next->value, "return") == 0) {
                    emit_blank_line(&st);
                }
            }
            continue;
        }

        /* 5. Semicolon ; */
        if (tok->type == TOKEN_PUNCTUATION && strcmp(tok->value, ";") == 0) {
            sb_append_char(&sb, ';');

            if (st.for_paren_depth > 0) {
                /* Inside 'for (init; cond; step)' header: emit space, not newline */
                sb_append_char(&sb, ' ');
            } else {
                emit_newline(&st);
                if (st.indent_level == 0) {
                    /* Blank line between top-level declarations and functions */
                    if (next && next->type != TOKEN_EOF) {
                        emit_blank_line(&st);
                    }
                } else if (next) {
                    if (next->type == TOKEN_KEYWORD &&
                        (strcmp(next->value, "if") == 0 ||
                         strcmp(next->value, "for") == 0 ||
                         strcmp(next->value, "while") == 0 ||
                         strcmp(next->value, "switch") == 0 ||
                         strcmp(next->value, "return") == 0)) {
                        /* Check if prev was not a block closing or already blank */
                        emit_blank_line(&st);
                    } else if (next->leading_blank_lines > 0) {
                        emit_blank_line(&st);
                    }
                }
            }
            continue;
        }

        /* 6. Commas , */
        if (tok->type == TOKEN_PUNCTUATION && strcmp(tok->value, ",") == 0) {
            sb_append_char(&sb, ',');
            if (st.config->space_after_comma) {
                if (next && next->type != TOKEN_PUNCTUATION) {
                    sb_append_char(&sb, ' ');
                }
            }
            continue;
        }

        /* 7. Parentheses ( and ) */
        if (tok->type == TOKEN_PUNCTUATION && strcmp(tok->value, "(") == 0) {
            st.paren_depth++;
            if (diag) diag->paren_balance++;

            if (st.awaiting_for_paren) {
                st.for_paren_depth = 1;
                st.awaiting_for_paren = false;
            } else if (st.for_paren_depth > 0) {
                st.for_paren_depth++;
            }

            if (prev && token_is_control_keyword(prev)) {
                /* Control statements: 'if (', 'while (', 'switch (' */
                if (st.config->space_after_keyword && sb_last_char(&sb) != ' ') {
                    sb_append_char(&sb, ' ');
                }
            } else if (prev && prev->type == TOKEN_KEYWORD && strcmp(prev->value, "return") == 0) {
                sb_append_char(&sb, ' ');
            }
            sb_append_char(&sb, '(');
            continue;
        }

        if (tok->type == TOKEN_PUNCTUATION && strcmp(tok->value, ")") == 0) {
            st.paren_depth--;
            if (diag) diag->paren_balance--;

            if (st.for_paren_depth > 0) {
                st.for_paren_depth--;
            }

            sb_append_char(&sb, ')');
            continue;
        }

        /* 8. Brackets [ and ] */
        if (tok->type == TOKEN_PUNCTUATION && strcmp(tok->value, "[") == 0) {
            st.square_depth++;
            if (diag) diag->square_balance++;
            sb_append_char(&sb, '[');
            continue;
        }

        if (tok->type == TOKEN_PUNCTUATION && strcmp(tok->value, "]") == 0) {
            st.square_depth--;
            if (diag) diag->square_balance--;
            sb_append_char(&sb, ']');
            continue;
        }

        /* 9. Case and Default labels */
        if (tok->type == TOKEN_KEYWORD && (strcmp(tok->value, "case") == 0 || strcmp(tok->value, "default") == 0)) {
            if (!st.at_line_start) {
                emit_newline(&st);
            }
            st.in_case = true;
            emit_indent(&st);
            sb_append_str(&sb, tok->value);
            if (strcmp(tok->value, "case") == 0) {
                sb_append_char(&sb, ' ');
            }
            continue;
        }

        /* Colon : */
        if (tok->type == TOKEN_OPERATOR && strcmp(tok->value, ":") == 0) {
            if (st.ternary_depth > 0) {
                /* Ternary operator: ' ? a : b' */
                st.ternary_depth--;
                sb_append_str(&sb, " : ");
            } else {
                /* Label or case/default terminator */
                sb_append_char(&sb, ':');
                emit_newline(&st);
                st.in_case = false;
            }
            continue;
        }

        /* Question mark ? (Ternary operator) */
        if (tok->type == TOKEN_OPERATOR && strcmp(tok->value, "?") == 0) {
            st.ternary_depth++;
            sb_append_str(&sb, " ? ");
            continue;
        }

        /* 10. Control Keywords: if, for, while, switch, do, else */
        if (tok->type == TOKEN_KEYWORD) {
            if (strcmp(tok->value, "do") == 0) {
                st.prev_was_do = true;
            }
            if (strcmp(tok->value, "for") == 0) {
                st.awaiting_for_paren = true;
            }

            if (st.at_line_start) {
                emit_indent(&st);
            } else {
                /* e.g., '} else' or type specifier */
                ensure_space_before(&st);
            }

            sb_append_str(&sb, tok->value);

            /* If followed by next token, check spacing */
            if (token_is_control_keyword(tok)) {
                if (st.config->space_after_keyword) {
                    sb_append_char(&sb, ' ');
                }
            } else if (strcmp(tok->value, "return") == 0 || strcmp(tok->value, "sizeof") == 0) {
                if (next && strcmp(next->value, "(") != 0) {
                    sb_append_char(&sb, ' ');
                }
            } else if (token_is_type_keyword(tok)) {
                /* Space after type keyword e.g., 'int a' */
                if (next && next->type != TOKEN_OPERATOR && strcmp(next->value, "*") != 0 && strcmp(next->value, ")") != 0) {
                    sb_append_char(&sb, ' ');
                }
            }
            continue;
        }

        /* 11. Operators */
        if (tok->type == TOKEN_OPERATOR) {
            /* Structure pointer/member access: ptr->val or obj.val */
            if (strcmp(tok->value, "->") == 0 || strcmp(tok->value, ".") == 0) {
                sb_append_str(&sb, tok->value);
                continue;
            }

            /* Increment / Decrement */
            if (strcmp(tok->value, "++") == 0 || strcmp(tok->value, "--") == 0) {
                sb_append_str(&sb, tok->value);
                continue;
            }

            /* Logical NOT and Bitwise NOT */
            if (strcmp(tok->value, "!") == 0 || strcmp(tok->value, "~") == 0) {
                if (st.at_line_start) emit_indent(&st);
                sb_append_str(&sb, tok->value);
                continue;
            }

            /* Binary vs Unary Operators */
            bool is_binary = token_is_binary_operator(tok, prev);
            if (is_binary && strcmp(tok->value, "*") == 0 && i >= 2) {
                const Token *prev2 = &list->tokens[i - 2];
                if (prev2->type == TOKEN_KEYWORD &&
                    (strcmp(prev2->value, "struct") == 0 ||
                     strcmp(prev2->value, "union") == 0 ||
                     strcmp(prev2->value, "enum") == 0)) {
                    is_binary = false; /* It's a struct/union pointer like struct Item *item */
                }
            }

            if (is_binary) {
                if (st.config->space_around_binary) {
                    ensure_space_before(&st);
                    sb_append_str(&sb, tok->value);
                    sb_append_char(&sb, ' ');
                } else {
                    sb_append_str(&sb, tok->value);
                }
            } else {
                /* Unary operator (+, -, *, &) */
                if (st.at_line_start) {
                    emit_indent(&st);
                } else if (prev && (prev->type == TOKEN_KEYWORD || token_is_type_keyword(prev) ||
                           (i >= 2 && (strcmp(list->tokens[i - 2].value, "struct") == 0 ||
                                       strcmp(list->tokens[i - 2].value, "union") == 0 ||
                                       strcmp(list->tokens[i - 2].value, "enum") == 0)))) {
                    /* Type pointer declaration: int *p or struct Item *p */
                    sb_append_char(&sb, ' ');
                }
                sb_append_str(&sb, tok->value);
            }
            continue;
        }

        /* 12. Identifiers, Numbers, Strings, Characters, and other literals */
        if (st.at_line_start) {
            emit_indent(&st);
        } else {
            /* Check if space is needed between previous token and current */
            if (prev) {
                if (prev->type == TOKEN_IDENTIFIER || prev->type == TOKEN_NUMBER ||
                    prev->type == TOKEN_KEYWORD || prev->type == TOKEN_STRING_LITERAL ||
                    prev->type == TOKEN_CHAR_LITERAL) {
                    if (sb_last_char(&sb) != ' ') {
                        sb_append_char(&sb, ' ');
                    }
                }
            }
        }

        sb_append_str(&sb, tok->value);
    }

    /* Trailing newline at end of file */
    if (sb.length > 0 && sb_last_char(&sb) != '\n') {
        sb_append_char(&sb, '\n');
    }

    /* Diagnostics summary checks */
    if (diag) {
        if (diag->brace_balance > 0) {
            formatter_diagnostics_add_warning(diag, "End of file: %d unclosed opening brace(s) '{'.", diag->brace_balance);
        } else if (diag->brace_balance < 0) {
            formatter_diagnostics_add_warning(diag, "End of file: %d extra closing brace(s) '}'.", -diag->brace_balance);
        }
        if (diag->paren_balance != 0) {
            formatter_diagnostics_add_warning(diag, "End of file: Unbalanced parentheses '()' (net: %d).", diag->paren_balance);
        }
        if (diag->square_balance != 0) {
            formatter_diagnostics_add_warning(diag, "End of file: Unbalanced square brackets '[]' (net: %d).", diag->square_balance);
        }
    }

    char *result = sb_to_string(&sb);
    sb_free(&sb);
    return result;
}

char *format_source_code(const char *source, const FormatterConfig *config, FormatterDiagnostics *diag) {
    if (!source) return NULL;
    char err_msg[256] = "";
    TokenList *tokens = tokenize_source(source, err_msg, sizeof(err_msg));
    if (!tokens) {
        if (diag) {
            formatter_diagnostics_add_warning(diag, "Tokenizer error: %s", err_msg[0] ? err_msg : "Unknown error");
        }
        return NULL;
    }
    if (err_msg[0] != '\0' && diag) {
        formatter_diagnostics_add_warning(diag, "%s", err_msg);
    }

    char *formatted = format_tokens(tokens, config, diag);
    token_list_free(tokens);
    return formatted;
}
