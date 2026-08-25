/* options.h - command-line option parsing. */
#ifndef MKSLUG_OPTIONS_H
#define MKSLUG_OPTIONS_H

#include "mkslug/mkslug.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* actions */
    int show_help;
    int show_version;

    /* behavior */
    int unslug;         /* -U : unslugify instead of slugify              */
    ms_case_t casemode; /* -x/-l/-u/-t                                    */
    int casemode_set;   /* whether a case flag was given explicitly       */

    /* text mode */
    int no_clipboard;   /* -n : do not copy to clipboard                  */

    /* file/dir mode */
    int force_file;     /* -f : force file/dir mode                       */
    int do_rename;      /* -r : rename on disk                            */
    int recurse;        /* -R : recurse into directories                  */
    long max_depth;     /* -d : recursion depth limit, 0 = unlimited      */
    char *exclude_pat;  /* -X : comma-separated pattern spec (owned)      */
    char *include_pat;  /* -I : comma-separated pattern spec (owned)      */

    /* config */
    char *config_path;  /* -c : explicit config file (owned)              */

    /* input */
    ms_mode_t mode;     /* resolved/forced mode                           */
    const char *input;  /* positional argument (not owned)                */
    int have_input;     /* whether a positional argument was supplied     */
} ms_options_t;

/* Initialize options with defaults. */
void ms_options_init(ms_options_t *o);

/* Free any owned strings inside the options struct. */
void ms_options_free(ms_options_t *o);

/* Parse argv into `o`.
 * Returns 0 on success. On a usage error prints a diagnostic to stderr and
 * returns MS_EXIT_USAGE. If -h/-v were requested, sets the corresponding flag
 * and returns 0 (the caller acts on it). */
int ms_options_parse(int argc, char **argv, ms_options_t *o);

/* Print help / version to the given stream. */
void ms_print_help(void);
void ms_print_version(void);

#ifdef __cplusplus
}
#endif

#endif /* MKSLUG_OPTIONS_H */
