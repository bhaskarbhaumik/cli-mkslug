/* mkslug.h - shared types and constants for the mkslug tool. */
#ifndef MKSLUG_H
#define MKSLUG_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Fallback version if the build system did not inject one. */
#ifndef MKSLUG_VERSION
#define MKSLUG_VERSION "1.0.0"
#endif

/* Case conversion modes. */
typedef enum {
    MS_CASE_NOCHANGE = 0, /* leave case as-is           */
    MS_CASE_LOWER,        /* all lowercase (slug default) */
    MS_CASE_UPPER,        /* ALL UPPERCASE              */
    MS_CASE_TITLE         /* Title Case Each Word       */
} ms_case_t;

/* Operating mode for the positional argument. */
typedef enum {
    MS_MODE_AUTO = 0, /* decide from filesystem existence */
    MS_MODE_TEXT,     /* treat argument as literal text   */
    MS_MODE_FILE      /* treat argument as file/directory */
} ms_mode_t;

/* Exit codes (sysexits-inspired but intentionally small and stable). */
enum {
    MS_EXIT_OK = 0,
    MS_EXIT_USAGE = 2,   /* bad command line                 */
    MS_EXIT_ERROR = 1,   /* runtime error (I/O, encoding...) */
    MS_EXIT_NOTFOUND = 3 /* path required but missing        */
};

#ifdef __cplusplus
}
#endif

#endif /* MKSLUG_H */
