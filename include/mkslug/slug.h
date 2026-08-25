/* slug.h - core slugify / unslugify / case-conversion primitives.
 *
 * All strings are UTF-8. Returned strings are heap-allocated and owned by the
 * caller (free with free()). On failure NULL is returned. */
#ifndef MKSLUG_SLUG_H
#define MKSLUG_SLUG_H

#include "mkslug/mkslug.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Convert arbitrary text into a slug.
 *
 *   sep           separator character to join words (typically '-').
 *   casemode      case applied to the result.
 *   transliterate if non-zero, romanize non-ASCII letters to ASCII using ICU
 *                 ("Any-Latin; Latin-ASCII"); if zero, Unicode letters/digits
 *                 are preserved and only separators are normalized.
 *   collapse      if non-zero, runs of separators collapse to one.
 */
char *ms_slugify(const char *utf8, char sep, ms_case_t casemode,
                 int transliterate, int collapse);

/* Convert a slug back into human-readable text: separator characters ('-' and
 * '_') become spaces, runs collapse, and the result is trimmed. Case is applied
 * per `casemode` (use MS_CASE_NOCHANGE to preserve). */
char *ms_unslugify(const char *utf8, ms_case_t casemode);

/* Apply Unicode-aware case conversion to a UTF-8 string. */
char *ms_case_convert(const char *utf8, ms_case_t casemode);

#ifdef __cplusplus
}
#endif

#endif /* MKSLUG_SLUG_H */
