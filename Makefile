# ==============================================================================
# Makefile: Mini Source Code Formatter in C
# Supports macOS, Linux, and Windows (MinGW / MSYS2 / WSL)
# ==============================================================================

CC ?= clang
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -O2
INCLUDES = -Iinclude

SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin
TEST_DIR = tests
OUTPUT_DIR = output

# Detect OS
ifeq ($(OS),Windows_NT)
    TARGET = $(BIN_DIR)/mini_formatter.exe
    TEST_TARGET = $(BIN_DIR)/test_formatter.exe
    MKDIR_P = if not exist $(subst /,\,$(1)) mkdir $(subst /,\,$(1))
    RM = del /Q /F /S
    RMDIR = rmdir /S /Q
    RUN_CMD = $(TARGET)
    RUN_TEST_CMD = $(TEST_TARGET)
else
    TARGET = $(BIN_DIR)/mini_formatter
    TEST_TARGET = $(BIN_DIR)/test_formatter
    MKDIR_P = mkdir -p $(1)
    RM = rm -f
    RMDIR = rm -rf
    RUN_CMD = ./$(TARGET)
    RUN_TEST_CMD = ./$(TEST_TARGET)
endif

# Source & Object Files
SRCS = $(SRC_DIR)/formatter.c \
       $(SRC_DIR)/file_handler.c \
       $(SRC_DIR)/tokenizer.c \
       $(SRC_DIR)/statistics.c \
       $(SRC_DIR)/utils.c \
       $(TEST_DIR)/test_formatter.c

APP_SRCS = $(SRC_DIR)/main.c $(SRCS)
APP_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(filter $(SRC_DIR)/%.c, $(APP_SRCS))) \
           $(BUILD_DIR)/test_formatter_lib.o

TEST_SRCS = $(filter-out $(SRC_DIR)/main.c, $(SRCS))
TEST_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(filter $(SRC_DIR)/%.c, $(TEST_SRCS))) \
            $(BUILD_DIR)/test_runner_main.o

# Phony Targets
.PHONY: all run test clean sample help

# Default Target
all: $(TARGET) $(TEST_TARGET)

# Compile Object Files for Application
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@$(call MKDIR_P, $(BUILD_DIR))
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_formatter_lib.o: $(TEST_DIR)/test_formatter.c
	@$(call MKDIR_P, $(BUILD_DIR))
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_runner_main.o: $(TEST_DIR)/test_formatter.c
	@$(call MKDIR_P, $(BUILD_DIR))
	$(CC) $(CFLAGS) $(INCLUDES) -DTEST_RUNNER_MAIN -c $< -o $@

# Link Main Executable
$(TARGET): $(APP_OBJS)
	@$(call MKDIR_P, $(BIN_DIR))
	$(CC) $(CFLAGS) $^ -o $@
	@echo "Build successful: $(TARGET)"

# Link Test Executable
$(TEST_TARGET): $(TEST_OBJS)
	@$(call MKDIR_P, $(BIN_DIR))
	$(CC) $(CFLAGS) $^ -o $@
	@echo "Build successful: $(TEST_TARGET)"

# Run Interactive Application
run: $(TARGET)
	$(RUN_CMD)

# Run Automated Test Suite
test: $(TEST_TARGET)
	@$(call MKDIR_P, $(OUTPUT_DIR))
	$(RUN_TEST_CMD)

# Format Sample File directly via CLI
sample: $(TARGET)
	@$(call MKDIR_P, $(OUTPUT_DIR))
	$(RUN_CMD) input/sample.c -o output/formatted_output.c --diff --stats

# Clean Build Artifacts
clean:
	@echo "Cleaning build artifacts..."
	$(RMDIR) $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean completed."

# Display Help
help:
	@echo "Mini Source Code Formatter Build Commands:"
	@echo "  make         - Compile the main application and test suite"
	@echo "  make run     - Run interactive menu-driven formatter"
	@echo "  make test    - Build and execute the 13 automated tests"
	@echo "  make sample  - Format input/sample.c to output/formatted_output.c"
	@echo "  make clean   - Remove build and bin directories"
