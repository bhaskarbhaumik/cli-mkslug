# test/Makefile - self-contained test build.
#
# Can be invoked standalone (`make -C test check`) or from the top-level
# Makefile. Recomputes dependency flags via pkg-config so it does not rely on
# variables passed from the parent.

CC         ?= cc
PKG_CONFIG ?= pkg-config
MKSLUG     ?= $(CURDIR)/../mkslug

UNAME_S := $(shell uname -s 2>/dev/null || echo unknown)
# Keg-only icu4c on macOS: inject its pkgconfig dir into each pkg-config call
# (a plain `export PKG_CONFIG_PATH` is not seen by make's $(shell ...)).
ifeq ($(UNAME_S),Darwin)
  BREW_ICU := $(shell brew --prefix icu4c@78 2>/dev/null || brew --prefix icu4c 2>/dev/null)
endif
ifneq ($(BREW_ICU),)
  PC_PATH := $(BREW_ICU)/lib/pkgconfig
endif
PKG_CONFIG_RUN = PKG_CONFIG_PATH="$(PC_PATH):$$PKG_CONFIG_PATH" $(PKG_CONFIG)

ICU_CFLAGS   := $(shell $(PKG_CONFIG_RUN) --cflags icu-uc icu-i18n 2>/dev/null)
ICU_LIBS     := $(shell $(PKG_CONFIG_RUN) --libs   icu-uc icu-i18n 2>/dev/null)
PCRE2_CFLAGS := $(shell $(PKG_CONFIG_RUN) --cflags libpcre2-8 2>/dev/null)
PCRE2_LIBS   := $(shell $(PKG_CONFIG_RUN) --libs   libpcre2-8 2>/dev/null)
ICU_LIBS     := $(if $(strip $(ICU_LIBS)),$(ICU_LIBS),-licui18n -licuuc -licudata)
PCRE2_LIBS   := $(if $(strip $(PCRE2_LIBS)),$(PCRE2_LIBS),-lpcre2-8)

CPPFLAGS += -I../include $(ICU_CFLAGS) $(PCRE2_CFLAGS)
CFLAGS   ?= -std=c11 -O2 -Wall -Wextra
LDLIBS   += $(ICU_LIBS) $(PCRE2_LIBS)

# Library sources under test (everything except main.c).
LIBSRCS := ../src/slug.c ../src/util.c ../src/pattern.c ../src/config.c \
           ../src/options.c ../src/clipboard.c ../src/fileops.c

TESTS := test_slug test_pattern

.PHONY: check clean
check: $(TESTS)
	@echo "== unit: test_slug =="   && ./test_slug
	@echo "== unit: test_pattern ==" && ./test_pattern
	@echo "== cli tests =="          && MKSLUG='$(MKSLUG)' sh ./cli_test.sh

test_slug: test_slug.c $(LIBSRCS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

test_pattern: test_pattern.c $(LIBSRCS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(TESTS)
