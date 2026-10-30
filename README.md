# Mini Source Code Formatter Using C Language

[![Language](https://img.shields.io/badge/Language-C11%20%2F%20C17-blue.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Platform](https://img.shields.io/badge/Platform-macOS%20%7C%20Linux%20%7C%20Windows-brightgreen.svg)]()
[![Standard](https://img.shields.io/badge/Standard-K%26R%20Braces%20%7C%204--Space%20Indent-orange.svg)]()
[![Tests](https://img.shields.io/badge/Tests-13%2F13%20Passing-success.svg)]()

A robust, modular, and beginner-friendly command-line and interactive menu-driven C source code beautifier developed in standard C. It tokenizes, structures, and formats unformatted C code into clean, readable, K&R-styled code while strictly preserving functionality, strings, comments, and preprocessor directives.

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Problem Statement & Objectives](#problem-statement--objectives)
3. [Key Features](#key-features)
4. [Demonstration of Core C Concepts (Viva / Exam Guide)](#demonstration-of-core-c-concepts-viva--exam-guide)
5. [Architecture & Design](#architecture--design)
6. [Source Code Module Directory](#source-code-module-directory)
7. [The Formatting Algorithm](#the-formatting-algorithm)
8. [Sample Input & Expected Output](#sample-input--expected-output)
9. [Build & Execution Guide](#build--execution-guide)
   - [macOS](#macos)
   - [Linux](#linux)
   - [Windows](#windows)
10. [Automated Testing & Idempotency](#automated-testing--idempotency)
11. [Limitations & Conservative Fallbacks](#limitations--conservative-fallbacks)
12. [Future Improvements](#future-improvements)

---

## 1. Project Overview

In software development, unformatted or minified code severely impairs readability, debugging efficiency, and collaborative code reviews. Commercial formatters (such as `clang-format` or `uncrustify`) are massive multi-megabyte tools with complex AST (Abstract Syntax Tree) parsers. 

The **Mini Source Code Formatter** implements a lightweight, standalone lexical tokenizer and token-state formatting engine purely in standard C (C11/C17). It provides both an **interactive terminal menu** and a scriptable **CLI interface**.

```
==================================================
         MINI SOURCE CODE FORMATTER
               C LANGUAGE
==================================================
Current File: input/sample.c | Status: Unformatted
--------------------------------------------------
  1. Select Source Code File
  2. Format Source Code
  3. Preview Original Code
  4. Preview Formatted Code
  5. View Code Statistics
  6. Compare Original and Formatted
  7. Formatter Settings
  8. Save Formatted File
  9. Batch Format Multiple Files
 10. Run Automated Tests
  0. Exit
==================================================
```

---

## 2. Problem Statement & Objectives

### Problem Statement
Unformatted or condensed C source code often lacks proper whitespace, consistent indentation, and logical line breaks:
```c
#include<stdio.h>
int main(){int a=10;int b=20;if(a>b){printf("A");}else{printf("B");}return 0;}
```
Such code is tedious to read, error-prone during modification, and difficult to assess.

### Objectives
1. **Preserve Exact Semantics**: Never alter the executable logic, variable names, literals, or preprocessor behavior.
2. **Deterministic Formatting**: Implement uniform indentation (4 spaces or tabs), K&R brace placement, and operator whitespace normalization.
3. **Lexical Safety**: Guarantee that comments (`//` and `/* ... */`) and string/character literals (`"..."`, `'...'`) remain intact.
4. **Idempotence**: Guarantee that formatting an already formatted file causes no textual change (`format(format(x)) == format(x)`).
5. **Educational Excellence**: Serve as a reference C engineering project demonstrating dynamic memory management, pointer manipulation, structs, file handling, and modular decomposition.

---

## 3. Key Features

### Core Features
- **File Input & Validation**: Reads `.c` and `.h` files, safely validating path existence, file extensions, and file sizes.
- **Lexical Tokenizer**: Distinguishes keywords, identifiers, numeric constants, string literals, comments, operators, and preprocessors.
- **K&R Style Bracing**: Attaches opening braces `{` to control lines and function headers (`int main() {`), placing closing braces `}` on their own lines, while smoothly chaining `} else {` and `} while (...)`.
- **Operator Spacing**: Normalizes spaces around binary operators (`=`, `+`, `-`, `*`, `/`, `==`, `!=`, `<`, `>`, `&&`, `||`, etc.).
- **For-Loop Header Preservation**: Detects semicolons inside `for (int i = 0; i < n; i++)` headers, emitting `; ` instead of newline breaks.
- **Comment Preservation**: Retains single-line (`//`) and multi-line (`/* ... */`) comments with proper alignment.
- **Code Metrics**: Computes line counts, blank lines, comment lines, estimated function definitions, and before/after byte sizes.
- **Safe Output & Backups**: Writes to a separate file (default `output/formatted_output.c`) and prompts for safety backups (`.bak`) before overwriting.

### Advanced Enhancements
- **Customizable Settings**: Configurable indentation width (2, 4, 8 spaces), space vs tab mode, operator spacing toggles, and brace styles.
- **Side-by-Side Diff**: Terminal viewer displaying side-by-side synchronized lines comparing original vs formatted code.
- **Batch Processing**: Format multiple C source files in a single pass.
- **Syntax Balance Warnings**: Detects unbalanced braces `{...}`, parentheses `(...)`, and unclosed literals or comments.

---

## 4. Demonstration of Core C Concepts (Viva / Exam Guide)

This project was built to illustrate real-world, idiomatic C programming practices. Below is a map of syllabus topics to their exact project implementation:

| C Concept | File & Location | Purpose & Practical Significance |
| :--- | :--- | :--- |
| **Functions & Modular Design** | `src/*.c`, `include/*.h` | Codebase divided into cohesive modules (`tokenizer`, `formatter`, `file_handler`, `statistics`, `utils`). |
| **Pointers & Pointer Arithmetic** | `src/tokenizer.c`, `src/utils.c` | String traversal using raw character pointers (`const char *p`), boundary offsets, and string manipulation. |
| **Dynamic Memory Allocation** | `src/utils.c` (`safe_malloc`, `safe_realloc`) | Dynamic `StringBuilder` buffer expansion, token arrays that scale dynamically with source file length. |
| **Structures (`struct`)** | `include/*.h` | Grouping related data: `Token`, `TokenList`, `StringBuilder`, `FormatterConfig`, `CodeStatistics`, `AppContext`. |
| **Arrays & String Literals** | `src/tokenizer.c`, `src/formatter.c` | Fixed keyword lookup tables, token value buffers, delimiter extraction with `memcpy` and `snprintf`. |
| **File I/O Handling** | `src/file_handler.c` | Safe binary file reading (`fopen`, `fseek`, `ftell`, `fread`), writing (`fwrite`), directory verification, and `.bak` backups. |
| **Control Statements & Loops** | `src/formatter.c`, `src/main.c` | Lexer state machines, menu switch statements, nested block tracking, brace matching loops. |
| **Preprocessor & Makefiles** | `Makefile`, `include/*.h` | Include guards (`#ifndef`), conditional compilation (`#ifdef TEST_RUNNER_MAIN`), and cross-platform build orchestration. |

---

## 5. Architecture & Design

The application follows a pipeline architecture:

```
[Raw C File (.c / .h)]
         │
         ▼ (file_handler.c)
[Memory Buffer in Heap]
         │
         ▼ (tokenizer.c)
[Token Stream (TokenList)]  ───► [Diagnostics: Balance & Syntax Checks]
         │
         ▼ (formatter.c)
[State Machine & Indent Engine]
         │
         ▼ (utils.c / StringBuilder)
[Formatted Output Buffer]
         │
    ┌────┴───────────────────────────┐
    ▼                                ▼
[output/formatted_output.c]   [Side-by-Side Terminal Diff & Stats]
```

### Flowchart of the Formatting Pipeline

```
 +------------------+
 |  Raw C Source    |
 +--------+---------+
          |
          v
 +------------------+      Unterminated comment?
 |  Lexical Lexer   | ---> Unclosed quote?
 |  (tokenizer.c)   |      Record warning in Diagnostics
 +--------+---------+
          |
          v Token Stream
 +------------------+
 | Token Classifier | ---> Is Keyword? Control statement? Binary operator?
 +--------+---------+
          |
          v
 +------------------+      Track: indent_level, paren_depth,
 | Formatter State  |             for_paren_depth, in_case,
 |  (formatter.c)   |             at_line_start, prev_token
 +--------+---------+
          |
          v
 +------------------+      Appends indent, tokens, newlines,
 |  StringBuilder   | ---> and normalized operator spacing
 +--------+---------+
          |
          v
 +------------------+
 | Formatted File   |
 +------------------+
```

---

## 6. Source Code Module Directory

```
MiniSourceCodeFormatter/
│
├── src/
│   ├── main.c           # Interactive menu UI & CLI argument dispatcher
│   ├── formatter.c      # Formatting engine, state tracking, and brace manager
│   ├── file_handler.c   # File validation, buffered I/O, safety backups, batch processor
│   ├── tokenizer.c      # Lexical analyzer classifying C language tokens
│   ├── statistics.c     # Code metrics (LOC, blank, comments, functions, size)
│   └── utils.c          # StringBuilder, memory helpers, ANSI UI & terminal diff
│
├── include/
│   ├── formatter.h      # Formatter configurations, signatures, and diagnostics struct
│   ├── file_handler.h   # File validation and I/O interface declarations
│   ├── tokenizer.h      # TokenType enum, Token struct, and tokenizer prototypes
│   ├── statistics.h     # CodeStatistics struct and reporting prototypes
│   └── utils.h          # StringBuilder methods, colors, and string utility definitions
│
├── input/
│   ├── sample.c          # Primary unformatted sample test input
│   └── advanced_sample.c # Multi-feature sample (structs, loops, pointers, comments)
│
├── output/
│   └── formatted_output.c # Default formatted output destination
│
├── tests/
│   └── test_formatter.c # 13-case automated unit and integration test suite
│
├── Makefile             # Portable build automation script (macOS, Linux, Windows)
├── README.md            # Complete project documentation and viva preparation
└── .gitignore           # Git ignore list for compiler objects and binaries
```

---

## 7. The Formatting Algorithm

### 1. Lexical Tokenization (`tokenizer.c`)
The lexer iterates character-by-character through the source string:
- **Preprocessor Directives**: Detected by leading `#`. Traverses to the end of the line, respecting backslash newline continuations (`\` followed by `\n`).
- **Comments**: Distinguishes `//` single-line comments from `/* ... */` multi-line comments. Unclosed comments are flagged in diagnostics.
- **Literals**: Preserves `"..."` and `'...'` byte-for-byte, ignoring syntax symbols inside quotes (e.g. `printf("a; {b}")`).
- **Punctuation & Operators**: Differentiates 3-char (`<<=`, `>>=`), 2-char (`==`, `!=`, `<=`, `>=`, `&&`, `||`, `++`, `--`, `->`, `+=`), and 1-char symbols.
- **Operator Disambiguation**: Differentiates binary multiplication (`a * b`) from pointer dereferencing/declaration (`int *ptr`, `struct Item *item`, `*p = 10`) by checking the syntactic nature of preceding tokens.

### 2. State-Driven Indentation & Emitting (`formatter.c`)
- **Brace Management**:
  - `{`: In K&R mode, appends space before `{` on the same line as the control header (`if (...) {`), emits a newline, and increments `indent_level`.
  - `}`: Decrements `indent_level`, emits on a fresh line at the new indent level. If immediately followed by `else` or `while` (in `do-while`), chains smoothly (`} else {`).
  - Semicolons after structs: Formats `struct Node { ... };` correctly without breaking before the semicolon.
- **For-Loop Header Protection**:
  - Tracks `for_paren_depth`. When inside a `for (...)` parenthesis scope, semicolons `;` emit a space `; ` rather than a line break.
- **Blank Line Heuristics**:
  - Automatically inserts an empty blank line after preprocessor headers, between top-level function definitions, and before return or control blocks when transitioning from variable declarations.

---

## 8. Sample Input & Expected Output

### Input (`input/sample.c`)
```c
#include<stdio.h>
int main(){int a=10;int b=20;if(a>b){printf("A");}else{printf("B");}return 0;}
```

### Formatted Output (`output/formatted_output.c`)
```c
#include <stdio.h>

int main() {
    int a = 10;
    int b = 20;

    if (a > b) {
        printf("A");
    } else {
        printf("B");
    }

    return 0;
}
```

### Comparative Metrics Generated
```
========================================================================
                     SOURCE CODE METRICS COMPARISON                     
========================================================================
Metric                   | input/sample.c | output/formatted_output.c | Delta       
------------------------------------------------------------------------
Total Lines              | 2              | 14             | +12
Blank Lines              | 0              | 3              | +3
Comment Lines            | 0              | 0              | +0
Functions Count          | 1              | 1              | 0
Character Count          | 97             | 161            | +64
File Size (bytes)        | 97             | 161            | +64 bytes
------------------------------------------------------------------------
Size change: +65.98% due to formatting, whitespace normalization, and indent structure.
========================================================================
```

---

## 9. Build & Execution Guide

### Prerequisites
- A standard C11/C17 compiler: `gcc` or `clang`.
- Standard POSIX `make` (or `mingw32-make` on Windows).

### macOS
```bash
# 1. Clone / Navigate to project directory
cd MiniSourceCodeFormatter

# 2. Compile application and test suite
make

# 3. Format sample file directly
make sample

# 4. Run automated test suite
make test

# 5. Launch interactive menu
make run
```

### Linux (Ubuntu, Debian, Fedora, Arch)
```bash
# Ensure build essentials are installed
sudo apt-get install build-essential

# Compile and run
make
./bin/mini_formatter input/sample.c -o output/formatted_output.c --diff --stats
./bin/mini_formatter
```

### Windows (MinGW, MSYS2, or WSL)
```powershell
# Using MinGW / GCC
mingw32-make
bin\mini_formatter.exe input\sample.c -o output\formatted_output.c --diff --stats
bin\mini_formatter.exe
```

### CLI Command Options
```bash
# Custom 2-space indentation with side-by-side terminal comparison
./bin/mini_formatter input/sample.c -i 2 --diff

# Tab indentation with preview to stdout
./bin/mini_formatter input/sample.c -t --preview

# Safe overwriting with automatic backup creation
./bin/mini_formatter input/sample.c -o output/formatted_output.c --backup

# Batch format multiple files
./bin/mini_formatter --batch file1.c file2.c file3.c
```

---

## 10. Automated Testing & Idempotency

The test suite (`tests/test_formatter.c`) contains 13 automated test suites:

1. `test_simple_unformatted`: Validates exact output matching the prompt's specification.
2. `test_nested_if_else`: Checks correct 4-space indentation scaling across deeply nested branches.
3. `test_nested_loops_and_for_headers`: Verifies that `for (int i = 0; i < n; i++)` semicolons do not break onto newlines.
4. `test_function_declarations_and_definitions`: Validates prototypes vs function definitions.
5. `test_arrays_and_pointers`: Verifies pointer dereferencing, address-of `&`, and array initializers.
6. `test_strings_with_special_chars`: Confirms strings containing `{`, `}`, `;` and `//` are preserved verbatim.
7. `test_comments_preservation`: Validates single-line (`//`) and multi-line (`/* ... */`) comment retention.
8. `test_preprocessor_directives`: Ensures macros and `#include` statements format cleanly.
9. `test_empty_file_handling`: Ensures empty inputs do not crash or produce memory errors.
10. `test_file_handler_error_cases`: Validates non-existent file rejection and invalid extension handling.
11. `test_idempotency`: Verifies that `format(format(code)) == format(code)`.
12. `test_unbalanced_braces_warning`: Verifies that unbalanced braces generate helpful diagnostic warnings.
13. `test_compilation_and_execution`: Compiles both the original and formatted code using the local C compiler and verifies that their execution output is identical!

Run the test suite at any time using:
```bash
make test
```

---

## 11. Limitations & Conservative Fallbacks

As a lightweight source code formatter, this tool prioritizes stability and semantic preservation over complex language transformations:
1. **No Complete C Grammar AST**: It does not parse full C abstract syntax trees. Heavily obfuscated macros that redefine fundamental syntax (such as `#define OPEN {`) are preserved safely as tokens without structural inference.
2. **Conservative Line Breaking**: If a language construct cannot be parsed with 100% confidence, the formatter safely preserves the token stream without corrupting code.
3. **Diagnostics, Not Full Validation**: Unbalanced braces and parentheses are flagged as warnings, but the formatter does not claim to replace a full C syntax validator like GCC.

---

## 12. Future Improvements

- Support for formatting C++ class access specifiers (`public:`, `private:`).
- Configuration loading from a `.formatterrc` JSON or INI file.
- Support for GNU and BSD style brace placement conventions via runtime flags.
- Syntax highlighting in the terminal using ANSI 256-color palettes.

---

## Author & Academic Integrity
- **Project**: Mini Source Code Formatter Using C Language
- **Language Standard**: ISO/IEC 9899:2011 (C11)
- **License**: MIT Open Source License
