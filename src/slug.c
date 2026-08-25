/* slug.c - slugify / unslugify / case conversion, powered by ICU.
 *
 * Everything crosses the API boundary as UTF-8. Internally we decode to UTF-16
 * (ICU's UChar), operate at the code-point level, and re-encode. The optional
 * transliteration step romanizes any script to ASCII Latin using ICU's
 * "Any-Latin; Latin-ASCII" transform (e.g. Zurich, slava, dongjing). */
#include "mkslug/slug.h"
#include "mkslug/util.h"

#include <stdlib.h>
#include <string.h>

#include <unicode/utypes.h>
#include <unicode/ustring.h>
#include <unicode/uchar.h>
#include <unicode/utf16.h>
#include <unicode/utrans.h>

/* ---- growable UChar (UTF-16) vector ------------------------------------- */

typedef struct {
    UChar *data;
    int32_t len;
    int32_t cap;
} uvec_t;

static void uvec_init(uvec_t *v)
{
    v->cap = 32;
    v->len = 0;
    v->data = ms_xmalloc((size_t)v->cap * sizeof(UChar));
}

static void uvec_free(uvec_t *v)
{
    free(v->data);
    v->data = NULL;
}

static void uvec_reserve(uvec_t *v, int32_t extra)
{
    if (v->len + extra > v->cap) {
        while (v->len + extra > v->cap)
            v->cap *= 2;
        v->data = ms_xrealloc(v->data, (size_t)v->cap * sizeof(UChar));
    }
}

static void uvec_append_cp(uvec_t *v, UChar32 c)
{
    UBool err = 0;
    uvec_reserve(v, 2);
    U16_APPEND(v->data, v->len, v->cap, c, err);
    (void)err;
}

/* ---- UTF-8 <-> UTF-16 --------------------------------------------------- */

/* Decode UTF-8 to a freshly allocated, NUL-terminated UChar buffer.
 * Returns NULL on failure; *out_len (if non-NULL) receives the length. */
static UChar *utf8_to_u16(const char *utf8, int32_t *out_len)
{
    UErrorCode ec = U_ZERO_ERROR;
    int32_t need = 0;
    int32_t srclen = (int32_t)strlen(utf8);
    UChar *buf;

    u_strFromUTF8(NULL, 0, &need, utf8, srclen, &ec);
    if (ec != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(ec))
        return NULL;
    ec = U_ZERO_ERROR;
    buf = ms_xmalloc((size_t)(need + 1) * sizeof(UChar));
    u_strFromUTF8(buf, need + 1, NULL, utf8, srclen, &ec);
    if (U_FAILURE(ec)) {
        free(buf);
        return NULL;
    }
    if (out_len)
        *out_len = need;
    return buf;
}

/* Encode UTF-16 to a freshly allocated, NUL-terminated UTF-8 string. */
static char *u16_to_utf8(const UChar *u, int32_t ulen)
{
    UErrorCode ec = U_ZERO_ERROR;
    int32_t need = 0;
    char *buf;

    u_strToUTF8(NULL, 0, &need, u, ulen, &ec);
    if (ec != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(ec))
        return NULL;
    ec = U_ZERO_ERROR;
    buf = ms_xmalloc((size_t)need + 1);
    u_strToUTF8(buf, need + 1, NULL, u, ulen, &ec);
    if (U_FAILURE(ec)) {
        free(buf);
        return NULL;
    }
    return buf;
}

/* ---- transliteration ---------------------------------------------------- */

/* Romanize `in` (len chars) to ASCII Latin. Returns a new uvec; on any ICU
 * failure, falls back to a copy of the input so slugging still proceeds. */
static int transliterate_u16(const UChar *in, int32_t inlen, uvec_t *out)
{
    static const UChar kRules[] = {
        /* "Any-Latin; Latin-ASCII" */
        'A', 'n', 'y', '-', 'L', 'a', 't', 'i', 'n', ';', ' ',
        'L', 'a', 't', 'i', 'n', '-', 'A', 'S', 'C', 'I', 'I', 0
    };
    UErrorCode ec = U_ZERO_ERROR;
    UTransliterator *t;
    int32_t textLen, limit;
    int32_t cap;

    t = utrans_openU(kRules, -1, UTRANS_FORWARD, NULL, 0, NULL, &ec);
    if (U_FAILURE(ec) || t == NULL) {
        /* No transliterator available: copy through unchanged. */
        uvec_reserve(out, inlen + 1);
        memcpy(out->data, in, (size_t)inlen * sizeof(UChar));
        out->len = inlen;
        return 0;
    }

    /* Transliteration may grow the text; start generous and retry on overflow. */
    cap = inlen * 4 + 16;
    out->data = ms_xrealloc(out->data, (size_t)cap * sizeof(UChar));
    out->cap = cap;
    memcpy(out->data, in, (size_t)inlen * sizeof(UChar));
    textLen = inlen;
    limit = inlen;

    ec = U_ZERO_ERROR;
    utrans_transUChars(t, out->data, &textLen, out->cap, 0, &limit, &ec);
    if (ec == U_BUFFER_OVERFLOW_ERROR) {
        out->data = ms_xrealloc(out->data, (size_t)(textLen + 1) * sizeof(UChar));
        out->cap = textLen + 1;
        memcpy(out->data, in, (size_t)inlen * sizeof(UChar));
        textLen = inlen;
        limit = inlen;
        ec = U_ZERO_ERROR;
        utrans_transUChars(t, out->data, &textLen, out->cap, 0, &limit, &ec);
    }
    utrans_close(t);

    if (U_FAILURE(ec)) {
        uvec_reserve(out, inlen + 1);
        memcpy(out->data, in, (size_t)inlen * sizeof(UChar));
        out->len = inlen;
        return 0;
    }
    out->len = textLen;
    return 0;
}

/* ---- case helpers ------------------------------------------------------- */

static UChar32 apply_case_cp(UChar32 c, ms_case_t mode, int word_start)
{
    switch (mode) {
    case MS_CASE_LOWER:
        return u_tolower(c);
    case MS_CASE_UPPER:
        return u_toupper(c);
    case MS_CASE_TITLE:
        return word_start ? u_totitle(c) : u_tolower(c);
    case MS_CASE_NOCHANGE:
    default:
        return c;
    }
}

/* Full-string, locale-aware case conversion on a UChar buffer. */
static UChar *case_convert_u16(const UChar *src, int32_t srclen, ms_case_t mode,
                               int32_t *out_len)
{
    UErrorCode ec = U_ZERO_ERROR;
    int32_t need = 0;
    UChar *buf;

    if (mode == MS_CASE_NOCHANGE) {
        buf = ms_xmalloc((size_t)(srclen + 1) * sizeof(UChar));
        memcpy(buf, src, (size_t)srclen * sizeof(UChar));
        buf[srclen] = 0;
        if (out_len)
            *out_len = srclen;
        return buf;
    }

#define TRY(call)                                                              \
    do {                                                                       \
        ec = U_ZERO_ERROR;                                                     \
        need = (call);                                                         \
    } while (0)

    switch (mode) {
    case MS_CASE_UPPER:
        TRY(u_strToUpper(NULL, 0, src, srclen, "", &ec));
        break;
    case MS_CASE_TITLE:
        TRY(u_strToTitle(NULL, 0, src, srclen, NULL, "", &ec));
        break;
    case MS_CASE_LOWER:
    default:
        TRY(u_strToLower(NULL, 0, src, srclen, "", &ec));
        break;
    }
    if (ec != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(ec))
        return NULL;

    buf = ms_xmalloc((size_t)(need + 1) * sizeof(UChar));
    ec = U_ZERO_ERROR;
    switch (mode) {
    case MS_CASE_UPPER:
        u_strToUpper(buf, need + 1, src, srclen, "", &ec);
        break;
    case MS_CASE_TITLE:
        u_strToTitle(buf, need + 1, src, srclen, NULL, "", &ec);
        break;
    case MS_CASE_LOWER:
    default:
        u_strToLower(buf, need + 1, src, srclen, "", &ec);
        break;
    }
#undef TRY
    if (U_FAILURE(ec)) {
        free(buf);
        return NULL;
    }
    if (out_len)
        *out_len = need;
    return buf;
}

/* ---- public API --------------------------------------------------------- */

char *ms_slugify(const char *utf8, char sep, ms_case_t casemode,
                 int transliterate, int collapse)
{
    UChar *src;
    int32_t srclen = 0;
    uvec_t work;
    uvec_t out;
    const UChar *scan;
    int32_t scanlen, i;
    int have_content = 0, pending = 0, word_start = 1;
    char *result;

    if (!utf8)
        return NULL;

    src = utf8_to_u16(utf8, &srclen);
    if (!src)
        return NULL;

    uvec_init(&work);
    if (transliterate) {
        transliterate_u16(src, srclen, &work);
        scan = work.data;
        scanlen = work.len;
    } else {
        scan = src;
        scanlen = srclen;
    }

    uvec_init(&out);
    i = 0;
    while (i < scanlen) {
        UChar32 c;
        U16_NEXT(scan, i, scanlen, c);
        if (u_isalnum(c)) {
            if (have_content && pending > 0) {
                int emit = collapse ? 1 : pending;
                int k;
                for (k = 0; k < emit; k++)
                    uvec_append_cp(&out, (UChar32)(unsigned char)sep);
            }
            pending = 0;
            uvec_append_cp(&out, apply_case_cp(c, casemode, word_start));
            have_content = 1;
            word_start = 0;
        } else {
            if (have_content)
                pending++;
            word_start = 1;
        }
    }

    result = u16_to_utf8(out.data, out.len);

    uvec_free(&work);
    uvec_free(&out);
    free(src);
    return result;
}

char *ms_unslugify(const char *utf8, ms_case_t casemode)
{
    ms_buf_t spaced;
    int have = 0, pending = 0;
    const unsigned char *p;
    char *result;

    if (!utf8)
        return NULL;

    /* Replace runs of '-' / '_' with a single space; trim edges. These are
     * ASCII bytes and can never appear inside a UTF-8 multibyte sequence, so
     * byte-level scanning is safe. */
    ms_buf_init(&spaced);
    for (p = (const unsigned char *)utf8; *p; p++) {
        if (*p == '-' || *p == '_') {
            if (have)
                pending = 1;
        } else {
            if (pending && have) {
                ms_buf_putc(&spaced, ' ');
                pending = 0;
            }
            ms_buf_putc(&spaced, (char)*p);
            have = 1;
        }
    }

    result = ms_case_convert(spaced.data, casemode);
    ms_buf_free(&spaced);
    return result;
}

char *ms_case_convert(const char *utf8, ms_case_t casemode)
{
    UChar *src, *conv;
    int32_t srclen = 0, convlen = 0;
    char *result;

    if (!utf8)
        return NULL;
    if (casemode == MS_CASE_NOCHANGE)
        return ms_xstrdup(utf8);

    src = utf8_to_u16(utf8, &srclen);
    if (!src)
        return NULL;
    conv = case_convert_u16(src, srclen, casemode, &convlen);
    free(src);
    if (!conv)
        return NULL;
    result = u16_to_utf8(conv, convlen);
    free(conv);
    return result;
}
