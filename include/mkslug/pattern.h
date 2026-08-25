/* pattern.h - glob and regex pattern sets backed by PCRE2.
 *
 * A pattern spec is a comma-separated list where each element is either:
 *   @glob@   a shell-style glob (translated to a regex internally)
 *   /regex/  a POSIX-extended-style regular expression (PCRE2 semantics)
 *
 * Commas inside a pattern may be escaped with a backslash ("\,"). */
#ifndef MKSLUG_PATTERN_H
#define MKSLUG_PATTERN_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ms_patternset ms_patternset_t;

/* Compile a spec into a pattern set. On error returns NULL and, if `err` is
 * non-NULL, sets *err to a malloc'd message the caller must free. An empty or
 * NULL spec yields a valid empty set (matches nothing). */
ms_patternset_t *ms_patternset_compile(const char *spec, char **err);

/* Return 1 if `name` matches any pattern in the set, else 0. An empty set
 * never matches. Matching is applied to the (base) name, anchored full-string
 * for globs and unanchored for regexes (standard grep-like behavior). */
int ms_patternset_match(const ms_patternset_t *ps, const char *name);

/* Number of compiled patterns (0 for an empty set). */
int ms_patternset_count(const ms_patternset_t *ps);

void ms_patternset_free(ms_patternset_t *ps);

#ifdef __cplusplus
}
#endif

#endif /* MKSLUG_PATTERN_H */
