# ============================================================
# CANZ - CAN Log Compressor
# ============================================================

CC      := gcc
AR      := ar
PREFIX  := /usr/local

# --- Directories ---
SRC_DIR  := src
INC_DIR  := include
CLI_DIR  := cli
BUILD    := build
LIB_OUT  := $(BUILD)/lib
BIN_OUT  := $(BUILD)/bin

# --- Compiler flags ---
CFLAGS   := -Wall -Wextra -O3 -std=c11 -I$(INC_DIR) -I$(SRC_DIR)
LDFLAGS  := -lzstd

# --- Source files ---
LIB_SRCS := \
    $(SRC_DIR)/canz.c          \
    $(SRC_DIR)/canz_buffer.c   \
    $(SRC_DIR)/canz_config.c   \
    $(SRC_DIR)/canz_schema.c   \
    $(SRC_DIR)/canz_parser.c   \
    $(SRC_DIR)/canz_compress.c \
    $(SRC_DIR)/canz_block.c    \
    $(SRC_DIR)/canz_stream.c   \
    $(SRC_DIR)/canz_decompress.c

CLI_SRC  := $(CLI_DIR)/canz_cli.c

# --- Object files ---
LIB_OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD)/%.o, $(LIB_SRCS))
CLI_OBJ  := $(BUILD)/canz_cli.o

# --- Output targets ---
STATIC_LIB := $(LIB_OUT)/libcanz.a
SHARED_LIB := $(LIB_OUT)/libcanz.so
CLI_BIN    := $(BIN_OUT)/canz

# ============================================================
# Default target: build everything
# ============================================================
.PHONY: all
all: dirs $(STATIC_LIB) $(SHARED_LIB) $(CLI_BIN)
	@echo ""
	@echo "  Build completed:"
	@echo "    Static Library  : $(STATIC_LIB)"
	@echo "    Shared Library  : $(SHARED_LIB)"
	@echo "    CLI Tool        : $(CLI_BIN)"
	@echo ""

# ============================================================
# Directory creation
# ============================================================
.PHONY: dirs
dirs:
	@mkdir -p $(BUILD) $(LIB_OUT) $(BIN_OUT)

# ============================================================
# Library object compilation
# ============================================================
$(BUILD)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/canz.h $(SRC_DIR)/canz_internal.h
	@echo "  CC  $<"
	@$(CC) $(CFLAGS) -fPIC -c $< -o $@

# ============================================================
# Static library
# ============================================================
$(STATIC_LIB): $(LIB_OBJS)
	@echo "  AR  $@"
	@$(AR) rcs $@ $^

# ============================================================
# Shared library
# ============================================================
$(SHARED_LIB): $(LIB_OBJS)
	@echo "  LD  $@"
	@$(CC) -shared -o $@ $^ $(LDFLAGS)

# ============================================================
# CLI binary
# ============================================================
$(CLI_OBJ): $(CLI_SRC) $(INC_DIR)/canz.h
	@echo "  CC  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(CLI_BIN): $(CLI_OBJ) $(STATIC_LIB)
	@echo "  LD  $@"
	@$(CC) -o $@ $^ $(LDFLAGS)

# ============================================================
# Convenience targets
# ============================================================

# Build only the static library
.PHONY: lib
lib: dirs $(STATIC_LIB)

# Build only the CLI tool
.PHONY: cli
cli: dirs $(CLI_BIN)

# Install to $(PREFIX)
.PHONY: install
install: all
	install -d $(DESTDIR)$(PREFIX)/lib
	install -d $(DESTDIR)$(PREFIX)/include
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 644 $(STATIC_LIB)        $(DESTDIR)$(PREFIX)/lib/
	install -m 755 $(SHARED_LIB)        $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(INC_DIR)/canz.h    $(DESTDIR)$(PREFIX)/include/
	install -m 755 $(CLI_BIN)           $(DESTDIR)$(PREFIX)/bin/
	ldconfig
	@echo "Installation Completed: $(PREFIX)"

# Uninstall
.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/libcanz.a
	rm -f $(DESTDIR)$(PREFIX)/lib/libcanz.so
	rm -f $(DESTDIR)$(PREFIX)/include/canz.h
	rm -f $(DESTDIR)$(PREFIX)/bin/canz

# Run a quick compress/decompress roundtrip test
.PHONY: test
test: cli
	@echo "--- Test: compression ---"
	@cp config/canz.conf .
	@$(CLI_BIN) -c canz.conf --stats || true
	@echo "--- Test: decompression ---"
	@$(CLI_BIN) -d canz.conf --stats || true

# Clean build artefacts
.PHONY: clean
clean:
	@rm -rf $(BUILD)
	@echo "  Cleaned."

# Deep clean (also removes any leftover .canz / decompressed files)
.PHONY: distclean
distclean: clean
	@rm -f *.canz decompressed.log canz.conf

# Print build environment info
.PHONY: info
info:
	@echo "CC      = $(CC)"
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "PREFIX  = $(PREFIX)"
