#!/bin/sh
# dev-build.sh - build mkslug locally on macOS WITHOUT accepting the Xcode
# license, using Homebrew's LLVM clang + lld and an explicit SDK path.
#
# On a normally configured machine you do not need this: just run `make`,
# `cmake`, or `./configure`. This helper exists only for environments where
# `sudo xcodebuild -license accept` has not been run, which makes Apple's
# /usr/bin/{make,git,cc,ld} shims refuse to execute. It relies on:
#   - Homebrew LLVM   (brew install llvm)
#   - Homebrew lld    (brew install lld)
#   - Homebrew make   (brew install make -> `gmake`)
#   - icu4c and pcre2 (brew install icu4c pcre2)
#
# Usage:  sh scripts/dev-build.sh [make-target...]   (default target: all + check)
set -eu

BREW_PREFIX="$(brew --prefix)"
CLANG="$BREW_PREFIX/opt/llvm/bin/clang"
GMAKE="$BREW_PREFIX/bin/gmake"
SDK="$(xcrun --show-sdk-path 2>/dev/null || echo /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk)"

[ -x "$CLANG" ] || { echo "need Homebrew llvm: brew install llvm" >&2; exit 1; }
[ -x "$GMAKE" ] || { echo "need Homebrew make: brew install make" >&2; exit 1; }
[ -x "$BREW_PREFIX/bin/ld64.lld" ] || { echo "need lld: brew install lld" >&2; exit 1; }

ICU_LIB="$(brew --prefix icu4c@78 2>/dev/null || brew --prefix icu4c)/lib"
PCRE2_LIB="$(brew --prefix pcre2)/lib"
export DYLD_LIBRARY_PATH="$ICU_LIB:$PCRE2_LIB${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
export PATH="$BREW_PREFIX/bin:$PATH"

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

COMMON="CC=$CLANG OPT=-O2 -isysroot $SDK"
LDF="-isysroot $SDK -fuse-ld=lld -B$BREW_PREFIX/bin"

set -x
"$GMAKE" CC="$CLANG" OPT="-O2 -isysroot $SDK" LDFLAGS="$LDF" "${@:-all}"
if [ "$#" -eq 0 ]; then
    "$GMAKE" -C test check CC="$CLANG" \
        CFLAGS="-std=c11 -O2 -Wall -Wextra -isysroot $SDK" \
        LDFLAGS="$LDF" MKSLUG="$ROOT/mkslug"
fi
