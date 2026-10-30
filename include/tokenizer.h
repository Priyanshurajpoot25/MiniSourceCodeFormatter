/**
 * @file tokenizer.h
 * @brief Lexical tokenizer for C source code.
 *
 * Breaks raw C source code into a sequence of classified tokens while
 * preserving string/char literal contents, comments, and preprocessor directives.
 *
 * Part of Mini Source Code Formatter Using C Language.
 */

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @enum TokenType
 * @brief Categorization of C language tokens.
 */
typedef enum {
    TOKEN_UNKNOWN,
    TOKEN_PREPROCESSOR,     /**< Directives: #include, #define, #ifdef, etc. */
    TOKEN_KEYWORD,          /**< C keywords: if, for, while, return, int, etc. */
    TOKEN_IDENTIFIER,       /**< Variable, function, type identifiers */
    TOKEN_NUMBER,           /**< Numeric literals: 10, 3.14, 0xFF */
    TOKEN_STRING_LITERAL,   /**< String literals: "hello" */
    TOKEN_CHAR_LITERAL,     /**< Character literals: 'c', '\n' */
    TOKEN_COMMENT_SINGLE,   /**< Single line comment: // ... */
    TOKEN_COMMENT_MULTI,    /**< Multi-line comment: / * ... * / */
    TOKEN_OPERATOR,         /**< Operators: +, -, *, ==, <=, &&, ->, etc. */
    TOKEN_PUNCTUATION,      /**< Punctuators: (, ), {, }, [, ], ;, ,, :, ? */
    TOKEN_EOF               /**< End of source stream */
} TokenType;

/**
 * @struct Token
 * @brief Represents a single lexical token in the source file.
 */
typedef struct {
    TokenType type;              /**< Category of the token */
    char *value;                 /**< Textual content (null-terminated) */
    size_t length;               /**< Length of value */
    size_t line;                 /**< 1-based original line number */
    size_t col;                  /**< 1-based original column number */
    bool has_leading_newline;    /**< True if token had a preceding newline */
    int leading_blank_lines;     /**< Count of blank lines preceding this token */
} Token;

/**
 * @struct TokenList
 * @brief Dynamic array of tokens.
 */
typedef struct {
    Token *tokens;               /**< Array of tokens */
    size_t count;                /**< Number of active tokens */
    size_t capacity;             /**< Total capacity */
} TokenList;

/* Lifecycle Functions */
TokenList *token_list_create(void);
void token_list_free(TokenList *list);
void token_list_add(TokenList *list, TokenType type, const char *value, size_t len,
                    size_t line, size_t col, bool has_leading_nl, int leading_blank_lines);

/* Tokenizer Core Function */
TokenList *tokenize_source(const char *source, char *err_msg, size_t err_size);

/* Token Inspection & Classification Helpers */
const char *token_type_to_string(TokenType type);
bool is_c_keyword(const char *str);
bool token_is_control_keyword(const Token *tok);
bool token_is_type_keyword(const Token *tok);
bool token_is_assignment_operator(const Token *tok);
bool token_is_binary_operator(const Token *tok, const Token *prev);
bool token_is_unary_operator(const Token *tok, const Token *prev);

#endif /* TOKENIZER_H */
