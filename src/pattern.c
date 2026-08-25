/* pattern.c - include/exclude pattern sets backed by PCRE2.
 *
 * A spec is a comma-separated list. Each element is either a glob delimited by
 * '@' (e.g. @*.tmp@) or a regex delimited by '/' (e.g. /^.+\.docx?$/). Globs are
 * translated to PCRE2 syntax and anchored; regexes are used as-is (unanchored).
 * Commas inside an element may be escaped as "\,". */
#include "mkslug/pattern.h"
#include "mkslug/util.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

struct ms_patternset {
    pcre2_code **codes;
    int n;
};

/* Split `spec` on unescaped commas. Returns a NULL-terminated array of
 * malloc'd, trimmed, non-empty elements (with "\," reduced to ","). */
static char **split_spec(const char *spec, int *count)
{
    size_t cap = 8;
    int n = 0;
    char **out = ms_xmalloc(cap * sizeof(char *));
    ms_buf_t cur;
    const char *p = spec;

    ms_buf_init(&cur);
    for (;;) {
        if (*p == '\\' && p[1] == ',') {
            ms_buf_putc(&cur, ',');
            p += 2;
            continue;
        }
        if (*p == ',' || *p == '\0') {
            /* trim surrounding ASCII whitespace */
            char *s = cur.data;
            size_t len = cur.len;
            while (len && (s[len - 1] == ' ' || s[len - 1] == '\t'))
                s[--len] = '\0';
            while (*s == ' ' || *s == '\t')
                s++;
            if (*s) {
                if ((size_t)n + 1 >= cap) {
                    cap *= 2;
                    out = ms_xrealloc(out, cap * sizeof(char *));
                }
                out[n++] = ms_xstrdup(s);
            }
            ms_buf_free(&cur);
            if (*p == '\0')
                break;
            ms_buf_init(&cur);
            p++;
            continue;
        }
        ms_buf_putc(&cur, *p);
        p++;
    }
    out[n] = NULL;
    *count = n;
    return out;
}

/* Translate a shell glob into an anchored PCRE2 pattern. */
static char *glob_to_regex(const char *glob)
{
    ms_buf_t rx;
    const char *g = glob;

    ms_buf_init(&rx);
    ms_buf_putc(&rx, '^');
    for (; *g; g++) {
        unsigned char c = (unsigned char)*g;
        switch (c) {
        case '*':
            ms_buf_puts(&rx, ".*");
            break;
        case '?':
            ms_buf_putc(&rx, '.');
            break;
        case '[': {
            /* Copy a character class, mapping a leading '!' to '^'. */
            const char *q = g + 1;
            ms_buf_putc(&rx, '[');
            if (*q == '!' || *q == '^') {
                ms_buf_putc(&rx, '^');
                q++;
            }
            if (*q == ']') { /* literal ']' as first member */
                ms_buf_puts(&rx, "\\]");
                q++;
            }
            while (*q && *q != ']') {
                if (*q == '\\' && q[1]) {
                    ms_buf_putc(&rx, '\\');
                    ms_buf_putc(&rx, q[1]);
                    q += 2;
                    continue;
                }
                ms_buf_putc(&rx, *q);
                q++;
            }
            ms_buf_putc(&rx, ']');
            if (*q == ']')
                g = q; /* loop's g++ moves past ']' */
            else
                g = q - 1;
            break;
        }
        case '\\':
            if (g[1]) {
                ms_buf_putc(&rx, '\\');
                ms_buf_putc(&rx, g[1]);
                g++;
            } else {
                ms_buf_puts(&rx, "\\\\");
            }
            break;
        /* escape PCRE2 metacharacters that are literals in globs */
        case '.': case '^': case '$': case '+': case '(': case ')':
        case '{': case '}': case '|':
            ms_buf_putc(&rx, '\\');
            ms_buf_putc(&rx, (char)c);
            break;
        default:
            ms_buf_putc(&rx, (char)c);
            break;
        }
    }
    ms_buf_putc(&rx, '$');
    return rx.data; /* caller frees */
}

static pcre2_code *compile_one(const char *elem, char **err)
{
    size_t len = strlen(elem);
    char *pat = NULL;
    int owned = 0;
    uint32_t options = 0;
    pcre2_code *code;
    int errcode;
    PCRE2_SIZE erroffset;

    if (len >= 2 && elem[0] == '@' && elem[len - 1] == '@') {
        char *inner = ms_xmalloc(len - 1);
        memcpy(inner, elem + 1, len - 2);
        inner[len - 2] = '\0';
        pat = glob_to_regex(inner);
        free(inner);
        owned = 1;
    } else if (len >= 2 && elem[0] == '/' && strrchr(elem + 1, '/')) {
        /* /pattern/  or  /pattern/flags  (flags: i, m, s, x). */
        const char *close = strrchr(elem + 1, '/');
        size_t plen = (size_t)(close - (elem + 1));
        const char *fp;
        pat = ms_xmalloc(plen + 1);
        memcpy(pat, elem + 1, plen);
        pat[plen] = '\0';
        owned = 1;
        for (fp = close + 1; *fp; fp++) {
            switch (*fp) {
            case 'i': options |= PCRE2_CASELESS; break;
            case 'm': options |= PCRE2_MULTILINE; break;
            case 's': options |= PCRE2_DOTALL; break;
            case 'x': options |= PCRE2_EXTENDED; break;
            default:
                if (err) {
                    char buf[128];
                    snprintf(buf, sizeof buf,
                             "unknown regex flag '%c' in '%s'", *fp, elem);
                    *err = ms_xstrdup(buf);
                }
                free(pat);
                return NULL;
            }
        }
    } else {
        if (err) {
            char buf[256];
            snprintf(buf, sizeof buf,
                     "pattern '%s' must be a glob (@...@) or regex (/.../)",
                     elem);
            *err = ms_xstrdup(buf);
        }
        return NULL;
    }

    code = pcre2_compile((PCRE2_SPTR)pat, PCRE2_ZERO_TERMINATED, options,
                         &errcode, &erroffset, NULL);
    if (!code && err) {
        PCRE2_UCHAR msg[256];
        pcre2_get_error_message(errcode, msg, sizeof msg);
        char buf[512];
        snprintf(buf, sizeof buf, "invalid pattern '%s' at offset %d: %s", pat,
                 (int)erroffset, (const char *)msg);
        *err = ms_xstrdup(buf);
    }
    if (owned)
        free(pat);
    return code;
}

ms_patternset_t *ms_patternset_compile(const char *spec, char **err)
{
    ms_patternset_t *ps = ms_xmalloc(sizeof *ps);
    char **elems;
    int n = 0, i;

    ps->codes = NULL;
    ps->n = 0;
    if (err)
        *err = NULL;
    if (!spec || !*spec)
        return ps;

    elems = split_spec(spec, &n);
    ps->codes = ms_xmalloc((size_t)(n ? n : 1) * sizeof(pcre2_code *));
    for (i = 0; i < n; i++) {
        pcre2_code *code = compile_one(elems[i], err);
        if (!code) {
            int j;
            for (j = 0; j < ps->n; j++)
                pcre2_code_free(ps->codes[j]);
            free(ps->codes);
            free(ps);
            for (j = 0; j < n; j++)
                free(elems[j]);
            free(elems);
            return NULL;
        }
        ps->codes[ps->n++] = code;
    }
    for (i = 0; i < n; i++)
        free(elems[i]);
    free(elems);
    return ps;
}

int ms_patternset_match(const ms_patternset_t *ps, const char *name)
{
    int i, rc = 0;
    pcre2_match_data *md;

    if (!ps || ps->n == 0)
        return 0;
    for (i = 0; i < ps->n; i++) {
        md = pcre2_match_data_create_from_pattern(ps->codes[i], NULL);
        if (!md)
            continue;
        if (pcre2_match(ps->codes[i], (PCRE2_SPTR)name, PCRE2_ZERO_TERMINATED, 0,
                        0, md, NULL) >= 0)
            rc = 1;
        pcre2_match_data_free(md);
        if (rc)
            break;
    }
    return rc;
}

int ms_patternset_count(const ms_patternset_t *ps)
{
    return ps ? ps->n : 0;
}

void ms_patternset_free(ms_patternset_t *ps)
{
    int i;
    if (!ps)
        return;
    for (i = 0; i < ps->n; i++)
        pcre2_code_free(ps->codes[i]);
    free(ps->codes);
    free(ps);
}
