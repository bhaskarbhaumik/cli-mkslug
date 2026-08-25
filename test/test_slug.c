/* test_slug.c - unit tests for the slug/unslug/case primitives. */
#include "mkslug/slug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_total = 0;
static int g_fail = 0;

static void check_str(const char *got, const char *want, const char *what,
                      int line)
{
    g_total++;
    if (!got || strcmp(got, want) != 0) {
        g_fail++;
        printf("  FAIL [line %d] %s: got \"%s\" want \"%s\"\n", line, what,
               got ? got : "(null)", want);
    }
}

#define SLUG(in, sep, cm, tr, col)  ms_slugify((in), (sep), (cm), (tr), (col))
#define EXPECT(call, want)                                                     \
    do {                                                                       \
        char *r_ = (call);                                                     \
        check_str(r_, (want), #call, __LINE__);                                \
        free(r_);                                                              \
    } while (0)

int main(void)
{
    /* Basic slugify */
    EXPECT(SLUG("Hello, World!", '-', MS_CASE_LOWER, 1, 1), "hello-world");
    EXPECT(SLUG("  leading and trailing  ", '-', MS_CASE_LOWER, 1, 1),
           "leading-and-trailing");
    EXPECT(SLUG("a  --  b", '-', MS_CASE_LOWER, 1, 1), "a-b");
    EXPECT(SLUG("snake_case_here", '-', MS_CASE_LOWER, 1, 1), "snake-case-here");
    EXPECT(SLUG("Numbers 123 go", '-', MS_CASE_LOWER, 1, 1), "numbers-123-go");

    /* Transliteration (ICU Any-Latin; Latin-ASCII) */
    EXPECT(SLUG("caf\xC3\xA9", '-', MS_CASE_LOWER, 1, 1), "cafe");      /* café  */
    EXPECT(SLUG("Z\xC3\xBCrich", '-', MS_CASE_LOWER, 1, 1), "zurich"); /* Zürich */
    EXPECT(SLUG("na\xC3\xAFve", '-', MS_CASE_LOWER, 1, 1), "naive");   /* naïve */
    EXPECT(SLUG("Stra\xC3\x9F" "e 1", '-', MS_CASE_LOWER, 1, 1), "strasse-1"); /* Straße 1 */

    /* Case modes */
    EXPECT(SLUG("my cool post", '-', MS_CASE_TITLE, 1, 1), "My-Cool-Post");
    EXPECT(SLUG("HELLO world", '-', MS_CASE_UPPER, 1, 1), "HELLO-WORLD");
    EXPECT(SLUG("MixedCase Word", '-', MS_CASE_NOCHANGE, 1, 1), "MixedCase-Word");

    /* Alternate separator */
    EXPECT(SLUG("hello world", '_', MS_CASE_LOWER, 1, 1), "hello_world");

    /* Unslugify */
    EXPECT(ms_unslugify("hello-world", MS_CASE_NOCHANGE), "hello world");
    EXPECT(ms_unslugify("my_cool_post", MS_CASE_TITLE), "My Cool Post");
    EXPECT(ms_unslugify("--edge--case--", MS_CASE_NOCHANGE), "edge case");

    /* Case conversion */
    EXPECT(ms_case_convert("hello", MS_CASE_UPPER), "HELLO");
    EXPECT(ms_case_convert("HELLO", MS_CASE_LOWER), "hello");
    EXPECT(ms_case_convert("hello world", MS_CASE_TITLE), "Hello World");

    printf("test_slug: %d/%d passed\n", g_total - g_fail, g_total);
    return g_fail ? 1 : 0;
}
