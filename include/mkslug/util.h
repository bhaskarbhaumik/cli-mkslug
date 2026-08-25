/* util.h - small allocation and string helpers. */
#ifndef MKSLUG_UTIL_H
#define MKSLUG_UTIL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Allocation wrappers that abort() on out-of-memory. Never return NULL. */
void *ms_xmalloc(size_t n);
void *ms_xrealloc(void *p, size_t n);
char *ms_xstrdup(const char *s);

/* A simple growable byte buffer, always kept NUL-terminated. */
typedef struct {
    char *data;
    size_t len;
    size_t cap;
} ms_buf_t;

void ms_buf_init(ms_buf_t *b);
void ms_buf_free(ms_buf_t *b);
void ms_buf_append(ms_buf_t *b, const char *s, size_t n);
void ms_buf_putc(ms_buf_t *b, char c);
void ms_buf_puts(ms_buf_t *b, const char *s);

/* Read all of stdin into a freshly allocated NUL-terminated buffer.
 * Returns malloc'd string (caller frees); *out_len set if non-NULL. */
char *ms_read_all_stdin(size_t *out_len);

/* Trim trailing newline(s) in place; returns s. */
char *ms_chomp(char *s);

/* Case-insensitive ASCII string compare (portable strcasecmp). */
int ms_strcasecmp_ascii(const char *a, const char *b);

/* Split "name.ext" into stem and extension. The returned extension pointer
 * (including the leading '.') points inside `name`, or "" if none. `stem_len`
 * receives the length of the stem portion. A leading dot (dotfile) is NOT
 * treated as an extension separator. */
const char *ms_split_ext(const char *name, size_t *stem_len);

#ifdef __cplusplus
}
#endif

#endif /* MKSLUG_UTIL_H */
