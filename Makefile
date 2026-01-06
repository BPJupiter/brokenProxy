# --- Settings ---
TARGET_NAME   := brokenproxy
SRC_DIRS      := src lib
# Include paths for headers (looks in src and lib)
INC_DIRS      := src lib

# --- Compiler ---
CC            := gcc
STD_FLAGS     := -std=gnu89 -pedantic -Wall -Wextra -DPING
LDFLAGS       := -lm -lpthread

# --- Build Configuration ---
# Defaults to "debug" unless you type "make release"
BUILD_TYPE ?= debug

ifeq ($(BUILD_TYPE),debug)
    CFLAGS     := $(STD_FLAGS) -g -O0 -DC_MEMORY_DEBUG
    DIR_SUFFIX := /debug
else
    CFLAGS     := $(STD_FLAGS) -O2 -DNDEBUG
    DIR_SUFFIX := /release
endif

# --- Output Directories ---
BIN_DIR := bin$(DIR_SUFFIX)
OBJ_DIR := obj$(DIR_SUFFIX)

# --- Auto-Discovery ---
# Find all .c files in src and lib recursively (requires Linux/Unix 'find')
SRCS := $(shell find $(SRC_DIRS) -name '*.c')

# Convert src/file.c -> obj/debug/src/file.o
OBJS := $(SRCS:%=$(OBJ_DIR)/%.o)

# Dependency files (.d) created by the compiler to track header changes
DEPS := $(OBJS:.o=.d)

# Add -I flags for include directories
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# ==========================================
#              Targets
# ==========================================

# Main Build Target
$(BIN_DIR)/$(TARGET_NAME): $(OBJS)
	@mkdir -p $(dir $@)
	@echo "Linking $(BUILD_TYPE) target: $@"
	@$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compile Step
# -MMD -MP generates the .d dependency files automatically
$(OBJ_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS) $(INC_FLAGS) -MMD -MP -c $< -o $@

# --- Phony Targets ---

.PHONY: all clean run debug release

all: $(BIN_DIR)/$(TARGET_NAME)

# Alias to force debug build
debug:
	@$(MAKE) --no-print-directory BUILD_TYPE=debug

# Alias to force release build
release:
	@$(MAKE) --no-print-directory BUILD_TYPE=release

# Run the executable after building
run: all
	@echo "Running $(BIN_DIR)/$(TARGET_NAME)..."
	@./$(BIN_DIR)/$(TARGET_NAME)

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf bin obj

# Include the dependency files so Make knows about header changes
-include $(DEPS)
