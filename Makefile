# Makefile - hand-written GNU make build for mkslug.
#
# Works with both GCC and Clang. Dependencies (ICU, PCRE2) are located via
# pkg-config. On macOS, Homebrew's keg-only icu4c pkgconfig directory is added
# to PKG_CONFIG_PATH automatically.
#
# Common targets:
#   make            build ./mkslug
#   make check      build and run the test suite
#   make install    install to $(PREFIX) (default /usr/local)
#   make clean      remove build artifacts
#   make format     run clang-format over the sources (if available)
#
# Useful overrides:
#   make CC=gcc-16
#   make CC=clang
#   make PREFIX=$HOME/.local install

# ---- toolchain ------------------------------------------------------------
CC         ?= cc
PKG_CONFIG ?= pkg-config
INSTALL    ?= install
CLANG_FORMAT ?= clang-format

VERSION := $(shell cat VERSION 2>/dev/null || echo 0.0.0)

UNAME_S := $(shell uname -s 2>/dev/null || echo unknown)

# On macOS, Homebrew's icu4c is keg-only, so its .pc files are not on the
# default pkg-config search path. Discover the prefix and inject it into
# PKG_CONFIG_PATH *inside each pkg-config invocation* below -- a plain
# `export PKG_CONFIG_PATH` does NOT reach make's $(shell ...) function.
ifeq ($(UNAME_S),Darwin)
  BREW_ICU := $(shell brew --prefix icu4c@78 2>/dev/null || brew --prefix icu4c 2>/dev/null)
endif
ifneq ($(BREW_ICU),)
  PC_PATH := $(BREW_ICU)/lib/pkgconfig
endif

# Run pkg-config with the keg-only path prepended, preserving any existing value.
PKG_CONFIG_RUN = PKG_CONFIG_PATH="$(PC_PATH):$$PKG_CONFIG_PATH" $(PKG_CONFIG)

# ---- dependency flags -----------------------------------------------------
ICU_CFLAGS   := $(shell $(PKG_CONFIG_RUN) --cflags icu-uc icu-i18n 2>/dev/null)
ICU_LIBS     := $(shell $(PKG_CONFIG_RUN) --libs   icu-uc icu-i18n 2>/dev/null)
PCRE2_CFLAGS := $(shell $(PKG_CONFIG_RUN) --cflags libpcre2-8 2>/dev/null)
PCRE2_LIBS   := $(shell $(PKG_CONFIG_RUN) --libs   libpcre2-8 2>/dev/null)

ifeq ($(strip $(ICU_LIBS)),)
  $(warning could not locate ICU via pkg-config; set PKG_CONFIG_PATH or install icu4c)
  ICU_LIBS := -licui18n -licuuc -licudata
endif
ifeq ($(strip $(PCRE2_LIBS)),)
  $(warning could not locate PCRE2 via pkg-config; install libpcre2)
  PCRE2_LIBS := -lpcre2-8
endif

# ---- build flags ----------------------------------------------------------
WARN     := -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes
STD      := -std=c11
OPT      ?= -O2
CPPFLAGS += -Iinclude -DMKSLUG_VERSION='"$(VERSION)"' $(ICU_CFLAGS) $(PCRE2_CFLAGS)
CFLAGS   ?= $(STD) $(OPT) $(WARN)
LDLIBS   += $(ICU_LIBS) $(PCRE2_LIBS)

# Windows clipboard backend needs user32.
ifneq (,$(findstring MINGW,$(UNAME_S)))
  LDLIBS += -luser32
endif
ifneq (,$(findstring MSYS,$(UNAME_S)))
  LDLIBS += -luser32
endif

# ---- install layout -------------------------------------------------------
PREFIX  ?= /usr/local
bindir  ?= $(PREFIX)/bin
datarootdir ?= $(PREFIX)/share
mandir  ?= $(datarootdir)/man

# ---- sources --------------------------------------------------------------
BIN      := mkslug
BUILDDIR := build
SRCS     := $(wildcard src/*.c)
OBJS     := $(patsubst src/%.c,$(BUILDDIR)/%.o,$(SRCS))
DEPS     := $(OBJS:.o=.d)

.PHONY: all clean check install uninstall format dirs
all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

$(BUILDDIR)/%.o: src/%.c | dirs
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

dirs:
	@mkdir -p $(BUILDDIR)

-include $(DEPS)

# ---- tests ----------------------------------------------------------------
# The test/ Makefile is self-contained (it recomputes ICU/PCRE2 flags and uses
# paths relative to test/), so only forward the compiler and binary path here.
# Forwarding the root CPPFLAGS/CFLAGS would leak root-relative -Iinclude paths.
check: $(BIN)
	@$(MAKE) -C test check CC='$(CC)' MKSLUG='$(CURDIR)/$(BIN)'

# ---- install --------------------------------------------------------------
install: $(BIN)
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) -m 0755 $(BIN) $(DESTDIR)$(bindir)/$(BIN)
	$(INSTALL) -d $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 0644 man/mkslug.1 $(DESTDIR)$(mandir)/man1/mkslug.1

uninstall:
	rm -f $(DESTDIR)$(bindir)/$(BIN)
	rm -f $(DESTDIR)$(mandir)/man1/mkslug.1

# ---- housekeeping ---------------------------------------------------------
format:
	@$(CLANG_FORMAT) --version >/dev/null 2>&1 || { echo "clang-format not found"; exit 0; }
	$(CLANG_FORMAT) -i $(SRCS) $(wildcard include/mkslug/*.h) $(wildcard test/*.c)

clean:
	rm -rf $(BUILDDIR) $(BIN)
	$(MAKE) -C test clean 2>/dev/null || true
