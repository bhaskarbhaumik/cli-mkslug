#!/bin/sh
# cli_test.sh - black-box tests driving the mkslug binary.
# Uses -n (no clipboard) so it runs cleanly in headless CI.
set -u

MKSLUG="${MKSLUG:-../mkslug}"
total=0
fail=0

check() {
    total=$((total + 1))
    _got="$1"
    _want="$2"
    if [ "$_got" != "$_want" ]; then
        fail=$((fail + 1))
        printf '  FAIL: got "%s" want "%s"\n' "$_got" "$_want"
    fi
}

# --- text mode ---
check "$("$MKSLUG" -n 'Hello, World!')" "hello-world"
check "$("$MKSLUG" -n -t 'my cool post')" "My-Cool-Post"
check "$("$MKSLUG" -n -u 'abc def')" "ABC-DEF"
check "$("$MKSLUG" -n -U 'hello-world')" "hello world"
check "$(printf 'Stra\303\237e 1' | "$MKSLUG" -n)" "strasse-1"
check "$(printf 'read from stdin' | "$MKSLUG" -n)" "read-from-stdin"

# --- version / help exit codes ---
"$MKSLUG" --version >/dev/null 2>&1 || { fail=$((fail+1)); echo "  FAIL: --version exit"; }
total=$((total+1))
"$MKSLUG" --help >/dev/null 2>&1 || { fail=$((fail+1)); echo "  FAIL: --help exit"; }
total=$((total+1))

# --- file mode: preview + rename ---
if command -v mktemp >/dev/null 2>&1; then
    tmp="$(mktemp -d)"
    if [ -n "$tmp" ] && [ -d "$tmp" ]; then
        # preview does not rename
        : > "$tmp/My Report.TXT"
        out="$("$MKSLUG" "$tmp/My Report.TXT")"
        total=$((total+1))
        case "$out" in
            *"my-report.TXT"*) : ;;
            *) fail=$((fail+1)); printf '  FAIL: preview missing slug: %s\n' "$out" ;;
        esac
        [ -f "$tmp/My Report.TXT" ] || { fail=$((fail+1)); echo "  FAIL: preview renamed file"; }
        total=$((total+1))

        # rename: extension preserved, stem slugified
        "$MKSLUG" -r "$tmp/My Report.TXT" >/dev/null
        total=$((total+1))
        [ -f "$tmp/my-report.TXT" ] || { fail=$((fail+1)); echo "  FAIL: rename did not produce my-report.TXT"; }

        # recursive rename with bottom-up directories
        mkdir -p "$tmp/Top Dir/Sub Dir"
        : > "$tmp/Top Dir/Sub Dir/Inner File.md"
        "$MKSLUG" -R -r "$tmp/Top Dir" >/dev/null
        total=$((total+1))
        if [ -f "$tmp/top-dir/sub-dir/inner-file.md" ]; then
            :
        else
            fail=$((fail+1)); echo "  FAIL: recursive rename tree not as expected"
            find "$tmp" 2>/dev/null | sed 's/^/    /'
        fi

        # case-only rename must work even on case-insensitive volumes
        # (destination "exists" but is the same object): XYZ -> xyz
        mkdir "$tmp/XYZ"
        "$MKSLUG" -r -l "$tmp/XYZ" >/dev/null 2>&1
        total=$((total+1))
        if [ -d "$tmp/xyz" ] && ls -f "$tmp" | grep -qx xyz; then
            :
        else
            fail=$((fail+1)); echo "  FAIL: case-only rename XYZ -> xyz did not apply"
        fi

        # a genuine collision with a different entry is still refused
        : > "$tmp/Doc One.txt"
        : > "$tmp/doc-one.txt"
        "$MKSLUG" -r -l "$tmp/Doc One.txt" >/dev/null 2>&1
        total=$((total+1))
        if [ -f "$tmp/Doc One.txt" ] && [ -f "$tmp/doc-one.txt" ]; then
            :
        else
            fail=$((fail+1)); echo "  FAIL: collision should have been refused"
        fi
        rm -rf "$tmp"
    fi
fi

echo "cli_test: $((total - fail))/$total passed"
[ "$fail" -eq 0 ]
