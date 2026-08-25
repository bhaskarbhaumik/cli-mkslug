/* util.c - allocation and string helpers. */
#include "mkslug/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *ms_xmalloc(size_t n)
{
    void *p = malloc(n ? n : 1);
    if (!p) {
        fputs("mkslug: out of memory\n", stderr);
        abort();
    }
    return p;
}

void *ms_xrealloc(void *p, size_t n)
{
    void *q = realloc(p, n ? n : 1);
    if (!q) {
        fputs("mkslug: out of memory\n", stderr);
        abort();
    }
    return q;
}

char *ms_xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = ms_xmalloc(n);
    memcpy(p, s, n);
    return p;
}

void ms_buf_init(ms_buf_t *b)
{
    b->cap = 32;
    b->data = ms_xmalloc(b->cap);
    b->data[0] = '\0';
    b->len = 0;
}

void ms_buf_free(ms_buf_t *b)
{
    free(b->data);
    b->data = NULL;
    b->len = b->cap = 0;
}

static void ms_buf_reserve(ms_buf_t *b, size_t extra)
{
    if (b->len + extra + 1 > b->cap) {
        while (b->len + extra + 1 > b->cap)
            b->cap *= 2;
        b->data = ms_xrealloc(b->data, b->cap);
    }
}

void ms_buf_append(ms_buf_t *b, const char *s, size_t n)
{
    ms_buf_reserve(b, n);
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

void ms_buf_putc(ms_buf_t *b, char c)
{
    ms_buf_reserve(b, 1);
    b->data[b->len++] = c;
    b->data[b->len] = '\0';
}

void ms_buf_puts(ms_buf_t *b, const char *s)
{
    ms_buf_append(b, s, strlen(s));
}

char *ms_read_all_stdin(size_t *out_len)
{
    size_t cap = 4096, len = 0;
    char *buf = ms_xmalloc(cap);
    size_t n;
    while ((n = fread(buf + len, 1, cap - len, stdin)) > 0) {
        len += n;
        if (len == cap) {
            cap *= 2;
            buf = ms_xrealloc(buf, cap);
        }
    }
    buf[len] = '\0';
    if (out_len)
        *out_len = len;
    return buf;
}

char *ms_chomp(char *s)
{
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r'))
        s[--n] = '\0';
    return s;
}

int ms_strcasecmp_ascii(const char *a, const char *b)
{
    unsigned char ca, cb;
    for (;;) {
        ca = (unsigned char)*a++;
        cb = (unsigned char)*b++;
        if (ca >= 'A' && ca <= 'Z')
            ca = (unsigned char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z')
            cb = (unsigned char)(cb - 'A' + 'a');
        if (ca != cb)
            return (int)ca - (int)cb;
        if (ca == '\0')
            return 0;
    }
}

const char *ms_split_ext(const char *name, size_t *stem_len)
{
    const char *dot = strrchr(name, '.');
    /* No dot, or a leading dot (dotfile like ".gitignore"): no extension. */
    if (!dot || dot == name) {
        *stem_len = strlen(name);
        return name + strlen(name); /* points at the terminating NUL: "" */
    }
    *stem_len = (size_t)(dot - name);
    return dot; /* includes the leading '.' */
}
