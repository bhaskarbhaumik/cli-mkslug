/* config.h - runtime configuration loaded from a config file. */
#ifndef MKSLUG_CONFIG_H
#define MKSLUG_CONFIG_H

#include "mkslug/mkslug.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Settings sourced from the configuration file (and compiled-in defaults).
 * These are overridden by command-line options where applicable. */
typedef struct {
    char separator;      /* word separator, default '-'                    */
    int clipboard;       /* copy slug to clipboard by default (1/0)        */
    int transliterate;   /* romanize non-ASCII via ICU (1/0), default 1    */
    ms_case_t default_case; /* default case in slug mode, default LOWER    */
    int collapse;        /* collapse repeated separators (1/0), default 1  */

    /* Defaults for command-line flags (a flag given on the command line wins). */
    int unslug;          /* -U : unslugify instead of slugify, default 0   */
    int force;           /* -f : force file/dir mode, default 0            */
    int rename;          /* -r : rename on disk, default 0                 */
    int recurse;         /* -R : recurse into directories, default 0       */
    long max_depth;      /* -d : recursion depth limit, 0 = unlimited      */
    char *exclude;       /* -X : pattern spec (owned), default MS_DEFAULT_EXCLUDE */
    char *include;       /* -I : pattern spec (owned), default NULL (none) */
} ms_config_t;

/* Compiled-in exclude spec: never touch macOS metadata or Git internals. */
#define MS_DEFAULT_EXCLUDE "@.DS_Store@,@.git@"

/* Initialize with compiled-in defaults. */
void ms_config_defaults(ms_config_t *cfg);

/* Free any owned strings inside the config struct. */
void ms_config_free(ms_config_t *cfg);

/* Load configuration.
 *
 * If `explicit_path` is non-NULL it is loaded and a failure to read it is an
 * error (returns -1). Otherwise the standard locations are tried in order:
 *   $XDG_CONFIG_HOME/mkslug/default.conf (or ~/.config/mkslug/default.conf)
 *   ~/.mkslug.conf
 * A missing default file is not an error. On success returns 0 and fills cfg.
 * On a syntax error, a message is printed and -1 is returned. */
int ms_config_load(ms_config_t *cfg, const char *explicit_path);

#ifdef __cplusplus
}
#endif

#endif /* MKSLUG_CONFIG_H */
