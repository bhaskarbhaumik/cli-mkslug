/* test_pattern.c - unit tests for glob/regex pattern sets. */
#include "mkslug/pattern.h"

#include <stdio.h>
#include <stdlib.h>

static int g_total = 0;
static int g_fail = 0;

static void check_match(ms_patternset_t *ps, const char *name, int want,
                        int line)
{
    int got = ms_patternset_match(ps, name);
    g_total++;
    if (got != want) {
        g_fail++;
        printf("  FAIL [line %d]: match(\"%s\") = %d, want %d\n", line, name,
               got, want);
    }
}

#define MATCH(ps, name, want) check_match((ps), (name), (want), __LINE__)

int main(void)
{
    char *err = NULL;
    ms_patternset_t *glob, *rx, *mixed;

    /* Glob set */
    glob = ms_patternset_compile("@*.tmp@,@*.exe@", &err);
    if (!glob) {
        printf("  FAIL: glob compile: %s\n", err ? err : "?");
        return 1;
    }
    MATCH(glob, "a.tmp", 1);
    MATCH(glob, "installer.exe", 1);
    MATCH(glob, "notes.txt", 0);
    MATCH(glob, "tmp", 0);
    ms_patternset_free(glob);

    /* Regex set (extended-style) */
    rx = ms_patternset_compile("/^.+\\.(doc|ppt|xls)x?$/", &err);
    if (!rx) {
        printf("  FAIL: regex compile: %s\n", err ? err : "?");
        return 1;
    }
    MATCH(rx, "report.doc", 1);
    MATCH(rx, "report.docx", 1);
    MATCH(rx, "slides.pptx", 1);
    MATCH(rx, "sheet.xls", 1);
    MATCH(rx, "archive.zip", 0);
    ms_patternset_free(rx);

    /* Mixed glob + regex */
    mixed = ms_patternset_compile("@*.log@,/^cache/", &err);
    if (!mixed) {
        printf("  FAIL: mixed compile: %s\n", err ? err : "?");
        return 1;
    }
    MATCH(mixed, "server.log", 1);
    MATCH(mixed, "cache_v2", 1);
    MATCH(mixed, "data.bin", 0);
    ms_patternset_free(mixed);

    /* Case-insensitive flag: /.../i */
    {
        ms_patternset_t *ci = ms_patternset_compile("/^report\\.docx?$/i", &err);
        if (!ci) {
            printf("  FAIL: /i compile: %s\n", err ? err : "?");
            return 1;
        }
        MATCH(ci, "report.docx", 1);
        MATCH(ci, "REPORT.DOCX", 1);
        MATCH(ci, "Report.Doc", 1);
        MATCH(ci, "report.pdf", 0);
        ms_patternset_free(ci);
    }

    /* Unknown flag is rejected. */
    {
        ms_patternset_t *badf = ms_patternset_compile("/abc/z", &err);
        g_total++;
        if (badf != NULL) {
            g_fail++;
            printf("  FAIL: expected NULL for unknown flag\n");
            ms_patternset_free(badf);
        }
        free(err);
        err = NULL;
    }

    /* Empty set matches nothing. */
    {
        ms_patternset_t *empty = ms_patternset_compile("", &err);
        MATCH(empty, "anything", 0);
        ms_patternset_free(empty);
    }

    /* Invalid element is rejected. */
    {
        ms_patternset_t *bad = ms_patternset_compile("not-delimited", &err);
        g_total++;
        if (bad != NULL) {
            g_fail++;
            printf("  FAIL: expected NULL for invalid pattern\n");
            ms_patternset_free(bad);
        }
        free(err);
        err = NULL;
    }

    printf("test_pattern: %d/%d passed\n", g_total - g_fail, g_total);
    return g_fail ? 1 : 0;
}
