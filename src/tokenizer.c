/**
 * @file tokenizer.c
 * @brief Implementation of C source code lexical tokenizer.
 *
 * Part of Mini Source Code Formatter Using C Language.
 */

#include "tokenizer.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* C Language Standard Keywords */
static const char *const C_KEYWORDS[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "inline", "int", "long", "register", "restrict", "return", "short",
    "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
    "unsigned", "void", "volatile", "while", "_Bool", "_Complex", "_Imaginary",
    "bool", "true", "false",
    /* Common standard library / C99+ types */
    "size_t", "ssize_t", "int8_t", "int16_t", "int32_t", "int64_t",
    "uint8_t", "uint16_t", "uint32_t", "uint64_t", "intptr_t", "uintptr_t",
    NULL
};

TokenList *token_list_create(void) {
    TokenList *list = (TokenList *)safe_malloc(sizeof(TokenList));
    list->capacity = 64;
    list->count = 0;
    list->tokens = (Token *)safe_malloc(sizeof(Token) * list->capacity);
    return list;
}

void token_list_free(TokenList *list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; ++i) {
        free(list->tokens[i].value);
    }
    free(list->tokens);
    free(list);
}

void token_list_add(TokenList *list, TokenType type, const char *value, size_t len,
                    size_t line, size_t col, bool has_leading_nl, int leading_blank_lines) {
    if (!list) return;
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->tokens = (Token *)safe_realloc(list->tokens, sizeof(Token) * list->capacity);
    }
    Token *tok = &list->tokens[list->count++];
    tok->type = type;
    tok->length = len;
    tok->value = (char *)safe_malloc(len + 1);
    if (len > 0 && value) {
        memcpy(tok->value, value, len);
    }
    tok->value[len] = '\0';
    tok->line = line;
    tok->col = col;
    tok->has_leading_newline = has_leading_nl;
    tok->leading_blank_lines = leading_blank_lines;
}

const char *token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_PREPROCESSOR:   return "PREPROCESSOR";
        case TOKEN_KEYWORD:        return "KEYWORD";
        case TOKEN_IDENTIFIER:     return "IDENTIFIER";
        case TOKEN_NUMBER:         return "NUMBER";
        case TOKEN_STRING_LITERAL: return "STRING_LITERAL";
        case TOKEN_CHAR_LITERAL:   return "CHAR_LITERAL";
        case TOKEN_COMMENT_SINGLE: return "COMMENT_SINGLE";
        case TOKEN_COMMENT_MULTI:  return "COMMENT_MULTI";
        case TOKEN_OPERATOR:       return "OPERATOR";
        case TOKEN_PUNCTUATION:    return "PUNCTUATION";
        case TOKEN_EOF:            return "EOF";
        default:                   return "UNKNOWN";
    }
}

bool is_c_keyword(const char *str) {
    if (!str) return false;
    for (int i = 0; C_KEYWORDS[i] != NULL; ++i) {
        if (strcmp(str, C_KEYWORDS[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool token_is_control_keyword(const Token *tok) {
    if (!tok || tok->type != TOKEN_KEYWORD) return false;
    const char *val = tok->value;
    return (strcmp(val, "if") == 0 ||
            strcmp(val, "for") == 0 ||
            strcmp(val, "while") == 0 ||
            strcmp(val, "switch") == 0);
}

bool token_is_type_keyword(const Token *tok) {
    if (!tok) return false;
    if (tok->type != TOKEN_KEYWORD) return false;
    const char *v = tok->value;
    return (strcmp(v, "int") == 0 || strcmp(v, "char") == 0 ||
            strcmp(v, "float") == 0 || strcmp(v, "double") == 0 ||
            strcmp(v, "void") == 0 || strcmp(v, "long") == 0 ||
            strcmp(v, "short") == 0 || strcmp(v, "signed") == 0 ||
            strcmp(v, "unsigned") == 0 || strcmp(v, "const") == 0 ||
            strcmp(v, "static") == 0 || strcmp(v, "volatile") == 0 ||
            strcmp(v, "struct") == 0 || strcmp(v, "enum") == 0 ||
            strcmp(v, "union") == 0 || strcmp(v, "typedef") == 0 ||
            strcmp(v, "bool") == 0 || strcmp(v, "size_t") == 0 ||
            strcmp(v, "uint8_t") == 0 || strcmp(v, "uint16_t") == 0 ||
            strcmp(v, "uint32_t") == 0 || strcmp(v, "uint64_t") == 0 ||
            strcmp(v, "int8_t") == 0 || strcmp(v, "int16_t") == 0 ||
            strcmp(v, "int32_t") == 0 || strcmp(v, "int64_t") == 0);
}

bool token_is_assignment_operator(const Token *tok) {
    if (!tok || tok->type != TOKEN_OPERATOR) return false;
    const char *v = tok->value;
    return (strcmp(v, "=") == 0 || strcmp(v, "+=") == 0 ||
            strcmp(v, "-=") == 0 || strcmp(v, "*=") == 0 ||
            strcmp(v, "/=") == 0 || strcmp(v, "%=") == 0 ||
            strcmp(v, "&=") == 0 || strcmp(v, "|=") == 0 ||
            strcmp(v, "^=") == 0 || strcmp(v, "<<=") == 0 ||
            strcmp(v, ">>=") == 0);
}

bool token_is_binary_operator(const Token *tok, const Token *prev) {
    if (!tok || tok->type != TOKEN_OPERATOR) return false;
    const char *v = tok->value;

    /* Member access operators are binary in grammar, but formatted with 0 spaces */
    if (strcmp(v, "->") == 0 || strcmp(v, ".") == 0) return false;

    /* Increment / Decrement are always unary */
    if (strcmp(v, "++") == 0 || strcmp(v, "--") == 0) return false;

    /* Logical NOT and Bitwise NOT are always unary */
    if (strcmp(v, "!") == 0 || strcmp(v, "~") == 0) return false;

    /* Assignment operators are always binary */
    if (token_is_assignment_operator(tok)) return true;

    /* Comparison operators are always binary */
    if (strcmp(v, "==") == 0 || strcmp(v, "!=") == 0 ||
        strcmp(v, "<=") == 0 || strcmp(v, ">=") == 0 ||
        strcmp(v, "<") == 0  || strcmp(v, ">") == 0) {
        return true;
    }

    /* Logical operators are always binary */
    if (strcmp(v, "&&") == 0 || strcmp(v, "||") == 0) return true;

    /* Arithmetic and bitwise shifts are always binary */
    if (strcmp(v, "/") == 0 || strcmp(v, "%") == 0 ||
        strcmp(v, "<<") == 0 || strcmp(v, ">>") == 0 ||
        strcmp(v, "^") == 0 || strcmp(v, "|") == 0) {
        return true;
    }

    /* Disambiguation for *, &, +, - */
    bool prev_is_operand = false;
    if (prev) {
        if (prev->type == TOKEN_IDENTIFIER ||
            prev->type == TOKEN_NUMBER ||
            prev->type == TOKEN_STRING_LITERAL ||
            prev->type == TOKEN_CHAR_LITERAL ||
            (prev->type == TOKEN_PUNCTUATION && (strcmp(prev->value, ")") == 0 || strcmp(prev->value, "]") == 0)) ||
            (prev->type == TOKEN_OPERATOR && (strcmp(prev->value, "++") == 0 || strcmp(prev->value, "--") == 0))) {
            prev_is_operand = true;
        }
    }

    if (strcmp(v, "+") == 0 || strcmp(v, "-") == 0 ||
        strcmp(v, "*") == 0 || strcmp(v, "&") == 0) {
        return prev_is_operand;
    }

    /* Ternary operator ? and : */
    if (strcmp(v, "?") == 0 || strcmp(v, ":") == 0) {
        return true;
    }

    return false;
}

bool token_is_unary_operator(const Token *tok, const Token *prev) {
    if (!tok || tok->type != TOKEN_OPERATOR) return false;
    const char *v = tok->value;
    if (strcmp(v, "!") == 0 || strcmp(v, "~") == 0 ||
        strcmp(v, "++") == 0 || strcmp(v, "--") == 0) {
        return true;
    }
    return !token_is_binary_operator(tok, prev);
}

/* ========================================================================= */
/* Tokenizer Core Implementation                                             */
/* ========================================================================= */

TokenList *tokenize_source(const char *source, char *err_msg, size_t err_size) {
    if (err_msg && err_size > 0) err_msg[0] = '\0';
    if (!source) {
        if (err_msg && err_size > 0) snprintf(err_msg, err_size, "Null source code provided.");
        return NULL;
    }

    TokenList *list = token_list_create();
    const char *p = source;
    size_t line = 1;
    size_t col = 1;

    bool has_leading_nl = false;
    int blank_lines = 0;
    int newline_count_in_ws = 0;

    while (*p) {
        /* 1. Handle Whitespace and track newlines/blank lines */
        if (isspace((unsigned char)*p)) {
            newline_count_in_ws = 0;
            while (*p && isspace((unsigned char)*p)) {
                if (*p == '\n') {
                    line++;
                    col = 1;
                    newline_count_in_ws++;
                } else {
                    col++;
                }
                p++;
            }
            if (newline_count_in_ws > 0) {
                has_leading_nl = true;
                if (newline_count_in_ws > 1) {
                    blank_lines += (newline_count_in_ws - 1);
                }
            }
            continue;
        }

        size_t tok_line = line;
        size_t tok_col = col;

        /* 2. Preprocessor Directives: begins with '#' when at line start or after whitespace */
        if (*p == '#') {
            const char *start = p;
            while (*p && *p != '\n') {
                /* Check for line continuation with backslash */
                if (*p == '\\' && *(p + 1) == '\n') {
                    p += 2;
                    line++;
                    col = 1;
                    continue;
                }
                p++;
                col++;
            }
            size_t len = (size_t)(p - start);
            token_list_add(list, TOKEN_PREPROCESSOR, start, len, tok_line, tok_col, has_leading_nl, blank_lines);
            has_leading_nl = false;
            blank_lines = 0;
            continue;
        }

        /* 3. Single-line and Multi-line Comments */
        if (*p == '/' && *(p + 1) == '/') {
            const char *start = p;
            p += 2;
            col += 2;
            while (*p && *p != '\n') {
                p++;
                col++;
            }
            size_t len = (size_t)(p - start);
            token_list_add(list, TOKEN_COMMENT_SINGLE, start, len, tok_line, tok_col, has_leading_nl, blank_lines);
            has_leading_nl = false;
            blank_lines = 0;
            continue;
        }

        if (*p == '/' && *(p + 1) == '*') {
            const char *start = p;
            p += 2;
            col += 2;
            bool closed = false;
            while (*p) {
                if (*p == '*' && *(p + 1) == '/') {
                    p += 2;
                    col += 2;
                    closed = true;
                    break;
                }
                if (*p == '\n') {
                    line++;
                    col = 1;
                } else {
                    col++;
                }
                p++;
            }
            if (!closed) {
                if (err_msg && err_size > 0) {
                    snprintf(err_msg, err_size, "Line %zu, Col %zu: Unterminated multi-line comment /* ...", tok_line, tok_col);
                }
            }
            size_t len = (size_t)(p - start);
            token_list_add(list, TOKEN_COMMENT_MULTI, start, len, tok_line, tok_col, has_leading_nl, blank_lines);
            has_leading_nl = false;
            blank_lines = 0;
            continue;
        }

        /* 4. String Literals */
        if (*p == '"') {
            const char *start = p;
            p++;
            col++;
            bool escaped = false;
            bool closed = false;
            while (*p) {
                if (*p == '\n') {
                    line++;
                    col = 1;
                } else {
                    col++;
                }
                if (escaped) {
                    escaped = false;
                } else if (*p == '\\') {
                    escaped = true;
                } else if (*p == '"') {
                    p++;
                    closed = true;
                    break;
                }
                p++;
            }
            if (!closed && err_msg && err_size > 0 && err_msg[0] == '\0') {
                snprintf(err_msg, err_size, "Line %zu, Col %zu: Unterminated string literal.", tok_line, tok_col);
            }
            size_t len = (size_t)(p - start);
            token_list_add(list, TOKEN_STRING_LITERAL, start, len, tok_line, tok_col, has_leading_nl, blank_lines);
            has_leading_nl = false;
            blank_lines = 0;
            continue;
        }

        /* 5. Character Literals */
        if (*p == '\'') {
            const char *start = p;
            p++;
            col++;
            bool escaped = false;
            bool closed = false;
            while (*p) {
                if (*p == '\n') {
                    line++;
                    col = 1;
                } else {
                    col++;
                }
                if (escaped) {
                    escaped = false;
                } else if (*p == '\\') {
                    escaped = true;
                } else if (*p == '\'') {
                    p++;
                    closed = true;
                    break;
                }
                p++;
            }
            if (!closed && err_msg && err_size > 0 && err_msg[0] == '\0') {
                snprintf(err_msg, err_size, "Line %zu, Col %zu: Unterminated character literal.", tok_line, tok_col);
            }
            size_t len = (size_t)(p - start);
            token_list_add(list, TOKEN_CHAR_LITERAL, start, len, tok_line, tok_col, has_leading_nl, blank_lines);
            has_leading_nl = false;
            blank_lines = 0;
            continue;
        }

        /* 6. Numbers (integers, hex, floats, etc.) */
        if (isdigit((unsigned char)*p) || (*p == '.' && isdigit((unsigned char)*(p + 1)))) {
            const char *start = p;
            if (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
                p += 2;
                col += 2;
                while (*p && (isxdigit((unsigned char)*p) || *p == '.')) {
                    p++;
                    col++;
                }
            } else {
                bool has_dot = false;
                bool has_exp = false;
                while (*p) {
                    if (*p == '.') {
                        if (has_dot) break;
                        has_dot = true;
                    } else if (*p == 'e' || *p == 'E') {
                        if (has_exp) break;
                        has_exp = true;
                        if (*(p + 1) == '+' || *(p + 1) == '-') {
                            p++;
                            col++;
                        }
                    } else if (!isdigit((unsigned char)*p)) {
                        break;
                    }
                    p++;
                    col++;
                }
            }
            /* Collect suffixes like u, l, ll, f, etc. */
            while (*p && (isalpha((unsigned char)*p) || *p == '_')) {
                p++;
                col++;
            }
            size_t len = (size_t)(p - start);
            token_list_add(list, TOKEN_NUMBER, start, len, tok_line, tok_col, has_leading_nl, blank_lines);
            has_leading_nl = false;
            blank_lines = 0;
            continue;
        }

        /* 7. Identifiers and Keywords */
        if (isalpha((unsigned char)*p) || *p == '_') {
            const char *start = p;
            while (*p && (isalnum((unsigned char)*p) || *p == '_')) {
                p++;
                col++;
            }
            size_t len = (size_t)(p - start);
            char temp[128];
            if (len < sizeof(temp)) {
                memcpy(temp, start, len);
                temp[len] = '\0';
                TokenType type = is_c_keyword(temp) ? TOKEN_KEYWORD : TOKEN_IDENTIFIER;
                token_list_add(list, type, start, len, tok_line, tok_col, has_leading_nl, blank_lines);
            } else {
                char *dynamic_temp = (char *)safe_malloc(len + 1);
                memcpy(dynamic_temp, start, len);
                dynamic_temp[len] = '\0';
                TokenType type = is_c_keyword(dynamic_temp) ? TOKEN_KEYWORD : TOKEN_IDENTIFIER;
                free(dynamic_temp);
                token_list_add(list, type, start, len, tok_line, tok_col, has_leading_nl, blank_lines);
            }
            has_leading_nl = false;
            blank_lines = 0;
            continue;
        }

        /* 8. Multi-character Operators (3 chars) */
        if ((strncmp(p, "<<=", 3) == 0) || (strncmp(p, ">>=", 3) == 0)) {
            token_list_add(list, TOKEN_OPERATOR, p, 3, tok_line, tok_col, has_leading_nl, blank_lines);
            p += 3;
            col += 3;
            has_leading_nl = false;
            blank_lines = 0;
            continue;
        }

        /* 9. Multi-character Operators (2 chars) */
        if ((strncmp(p, "==", 2) == 0) || (strncmp(p, "!=", 2) == 0) ||
            (strncmp(p, "<=", 2) == 0) || (strncmp(p, ">=", 2) == 0) ||
            (strncmp(p, "&&", 2) == 0) || (strncmp(p, "||", 2) == 0) ||
            (strncmp(p, "++", 2) == 0) || (strncmp(p, "--", 2) == 0) ||
            (strncmp(p, "->", 2) == 0) || (strncmp(p, "<<", 2) == 0) ||
            (strncmp(p, ">>", 2) == 0) || (strncmp(p, "+=", 2) == 0) ||
            (strncmp(p, "-=", 2) == 0) || (strncmp(p, "*=", 2) == 0) ||
            (strncmp(p, "/=", 2) == 0) || (strncmp(p, "%=", 2) == 0) ||
            (strncmp(p, "&=", 2) == 0) || (strncmp(p, "|=", 2) == 0) ||
            (strncmp(p, "^=", 2) == 0)) {
            token_list_add(list, TOKEN_OPERATOR, p, 2, tok_line, tok_col, has_leading_nl, blank_lines);
            p += 2;
            col += 2;
            has_leading_nl = false;
            blank_lines = 0;
            continue;
        }

        /* 10. Single Character Punctuators */
        if (*p == '(' || *p == ')' || *p == '{' || *p == '}' ||
            *p == '[' || *p == ']' || *p == ';' || *p == ',') {
            token_list_add(list, TOKEN_PUNCTUATION, p, 1, tok_line, tok_col, has_leading_nl, blank_lines);
            p++;
            col++;
            has_leading_nl = false;
            blank_lines = 0;
            continue;
        }

        /* 11. Single Character Operators and Ternary Punctuators */
        if (*p == '=' || *p == '+' || *p == '-' || *p == '*' ||
            *p == '/' || *p == '%' || *p == '<' || *p == '>' ||
            *p == '!' || *p == '~' || *p == '&' || *p == '|' ||
            *p == '^' || *p == '.' || *p == '?' || *p == ':') {
            token_list_add(list, TOKEN_OPERATOR, p, 1, tok_line, tok_col, has_leading_nl, blank_lines);
            p++;
            col++;
            has_leading_nl = false;
            blank_lines = 0;
            continue;
        }

        /* 12. Fallback for unexpected characters */
        token_list_add(list, TOKEN_UNKNOWN, p, 1, tok_line, tok_col, has_leading_nl, blank_lines);
        p++;
        col++;
        has_leading_nl = false;
        blank_lines = 0;
    }

    /* Add EOF Token */
    token_list_add(list, TOKEN_EOF, "", 0, line, col, has_leading_nl, blank_lines);
    return list;
}
