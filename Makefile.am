# Makefile.am - automake build for mkslug (out-of-tree; see configure.ac).

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -I$(top_srcdir)/include $(ICU_CFLAGS) $(PCRE2_CFLAGS) \
              -DMKSLUG_VERSION='"$(PACKAGE_VERSION)"'
AM_CFLAGS = -Wall -Wextra

# Sources shared by the program and the unit tests (everything except main.c).
mkslug_common_sources = \
    src/options.c \
    src/config.c \
    src/slug.c \
    src/clipboard.c \
    src/pattern.c \
    src/fileops.c \
    src/util.c

bin_PROGRAMS = mkslug
mkslug_SOURCES = src/main.c $(mkslug_common_sources)
mkslug_LDADD = $(ICU_LIBS) $(PCRE2_LIBS)

dist_man1_MANS = man/mkslug.1

# ---- tests ----------------------------------------------------------------
check_PROGRAMS = test_slug test_pattern

test_slug_SOURCES = test/test_slug.c $(mkslug_common_sources)
test_slug_LDADD = $(ICU_LIBS) $(PCRE2_LIBS)

test_pattern_SOURCES = test/test_pattern.c $(mkslug_common_sources)
test_pattern_LDADD = $(ICU_LIBS) $(PCRE2_LIBS)

TEST_EXTENSIONS = .sh
SH_LOG_COMPILER = $(SHELL)
AM_TESTS_ENVIRONMENT = MKSLUG='$(abs_top_builddir)/mkslug'; export MKSLUG;

TESTS = test_slug test_pattern test/cli_test.sh

EXTRA_DIST = \
    README.md \
    LICENSE \
    VERSION \
    CMakeLists.txt \
    Makefile \
    mkslug-features.md \
    include/mkslug/version.h.in \
    config/mkslug.conf.example \
    test/cli_test.sh
