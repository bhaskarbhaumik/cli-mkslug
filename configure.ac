#                                               -*- Autoconf -*-
# configure.ac - autoconf/automake build for mkslug.
#
# NOTE: mkslug must be configured OUT OF TREE so the hand-written ./Makefile
# (the GNU make build) is never overwritten:
#
#     autoreconf --install
#     mkdir build && cd build && ../configure && make && make check

AC_PREREQ([2.69])
AC_INIT([mkslug],
        [m4_esyscmd_s([cat VERSION])],
        [https://example.org/mkslug/issues],
        [mkslug],
        [https://example.org/mkslug])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([m4])
AC_CANONICAL_HOST

# Refuse in-tree builds to protect the hand-written ./Makefile.
if test "`cd $srcdir && pwd`" = "`pwd`"; then
  AC_MSG_ERROR([in-tree builds are disabled to protect the hand-written ./Makefile.
    Please build out of tree, e.g.:
        mkdir build && cd build && ../configure])
fi

AM_INIT_AUTOMAKE([1.16 foreign subdir-objects -Wall -Werror])
AM_SILENT_RULES([yes])

AC_PROG_CC
AC_PROG_INSTALL

# Prefer C11 if the compiler supports it (no autoconf-archive dependency).
AC_MSG_CHECKING([whether $CC accepts -std=c11])
ms_save_CFLAGS=$CFLAGS
CFLAGS="$CFLAGS -std=c11"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
  [AC_MSG_RESULT([yes])],
  [AC_MSG_RESULT([no]); CFLAGS=$ms_save_CFLAGS])

# On macOS, Homebrew's icu4c is keg-only; make sure pkg-config can see it.
case "$host_os" in
  darwin*)
    if command -v brew >/dev/null 2>&1; then
      icu_prefix=`brew --prefix icu4c@78 2>/dev/null`
      if test -z "$icu_prefix"; then
        icu_prefix=`brew --prefix icu4c 2>/dev/null`
      fi
      if test -n "$icu_prefix"; then
        PKG_CONFIG_PATH="$icu_prefix/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
        export PKG_CONFIG_PATH
      fi
    fi
    ;;
esac

PKG_PROG_PKG_CONFIG([0.24])
PKG_CHECK_MODULES([ICU], [icu-uc icu-i18n],, [
  AC_MSG_ERROR([ICU (icu-uc, icu-i18n) not found. Install icu4c / libicu-dev.])
])
PKG_CHECK_MODULES([PCRE2], [libpcre2-8],, [
  AC_MSG_ERROR([PCRE2 (libpcre2-8) not found. Install pcre2 / libpcre2-dev.])
])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

AC_MSG_NOTICE([

  mkslug $PACKAGE_VERSION configured.
    CC .......... $CC
    ICU ......... $ICU_CFLAGS
    PCRE2 ....... $PCRE2_CFLAGS
    prefix ...... $prefix

  Now run: make && make check
])
