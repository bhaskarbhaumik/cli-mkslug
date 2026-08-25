/* options.c - command-line parsing (portable, no getopt dependency). */
#include "mkslug/options.h"
#include "mkslug/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ms_options_init(ms_options_t *o)
{
    memset(o, 0, sizeof *o);
    o->casemode = MS_CASE_LOWER;
    o->casemode_set = 0;
    o->max_depth = 0;
    o->mode = MS_MODE_AUTO;
}

void ms_options_free(ms_options_t *o)
{
    free(o->exclude_pat);
    free(o->include_pat);
    free(o->config_path);
    o->exclude_pat = o->include_pat = o->config_path = NULL;
}

static void set_case(ms_options_t *o, ms_case_t c)
{
    if (!o->casemode_set) { /* first case option wins */
        o->casemode = c;
        o->casemode_set = 1;
    }
}

static int need_arg(const char *opt, const char *val)
{
    if (!val) {
        fprintf(stderr, "mkslug: option '%s' requires an argument\n", opt);
        return -1;
    }
    return 0;
}

static int parse_long(ms_options_t *o, const char *arg, int *iref, int argc,
                      char **argv)
{
    int i = *iref;
    const char *name = arg + 2;
    const char *eq = strchr(name, '=');
    char namebuf[64];
    const char *val = NULL;

    if (eq) {
        size_t n = (size_t)(eq - name);
        if (n >= sizeof namebuf)
            n = sizeof namebuf - 1;
        memcpy(namebuf, name, n);
        namebuf[n] = '\0';
        name = namebuf;
        val = eq + 1;
    }

#define WANT_VALUE()                                                           \
    do {                                                                       \
        if (!val) {                                                            \
            if (i + 1 < argc)                                                  \
                val = argv[++i];                                               \
        }                                                                      \
        if (need_arg(arg, val) != 0)                                           \
            return -1;                                                         \
    } while (0)

    if (!strcmp(name, "help"))
        o->show_help = 1;
    else if (!strcmp(name, "version"))
        o->show_version = 1;
    else if (!strcmp(name, "unslug"))
        o->unslug = 1;
    else if (!strcmp(name, "no-change"))
        set_case(o, MS_CASE_NOCHANGE);
    else if (!strcmp(name, "lower"))
        set_case(o, MS_CASE_LOWER);
    else if (!strcmp(name, "upper"))
        set_case(o, MS_CASE_UPPER);
    else if (!strcmp(name, "title"))
        set_case(o, MS_CASE_TITLE);
    else if (!strcmp(name, "no-clipboard"))
        o->no_clipboard = 1;
    else if (!strcmp(name, "force"))
        o->force_file = 1, o->mode = MS_MODE_FILE;
    else if (!strcmp(name, "rename"))
        o->do_rename = 1;
    else if (!strcmp(name, "recurse"))
        o->recurse = 1;
    else if (!strcmp(name, "config")) {
        WANT_VALUE();
        free(o->config_path);
        o->config_path = ms_xstrdup(val);
    } else if (!strcmp(name, "exclude")) {
        WANT_VALUE();
        free(o->exclude_pat);
        o->exclude_pat = ms_xstrdup(val);
    } else if (!strcmp(name, "include")) {
        WANT_VALUE();
        free(o->include_pat);
        o->include_pat = ms_xstrdup(val);
    } else if (!strcmp(name, "max-depth")) {
        char *end;
        WANT_VALUE();
        o->max_depth = strtol(val, &end, 10);
        if (*end != '\0' || o->max_depth < 0) {
            fprintf(stderr, "mkslug: invalid --max-depth '%s'\n", val);
            return -1;
        }
    } else {
        fprintf(stderr, "mkslug: unknown option '--%s'\n", name);
        return -1;
    }
#undef WANT_VALUE
    *iref = i;
    return 0;
}

static int parse_short(ms_options_t *o, const char *arg, int *iref, int argc,
                       char **argv)
{
    int i = *iref;
    const char *p = arg + 1;

    while (*p) {
        char c = *p;
        /* options that take a value */
        if (c == 'c' || c == 'd' || c == 'X' || c == 'I') {
            const char *val = NULL;
            char optname[3] = {'-', c, '\0'};
            if (p[1] != '\0') {
                val = p + 1; /* attached: -cfile */
            } else if (i + 1 < argc) {
                val = argv[++i];
            }
            if (need_arg(optname, val) != 0)
                return -1;
            switch (c) {
            case 'c':
                free(o->config_path);
                o->config_path = ms_xstrdup(val);
                break;
            case 'X':
                free(o->exclude_pat);
                o->exclude_pat = ms_xstrdup(val);
                break;
            case 'I':
                free(o->include_pat);
                o->include_pat = ms_xstrdup(val);
                break;
            case 'd': {
                char *end;
                o->max_depth = strtol(val, &end, 10);
                if (*end != '\0' || o->max_depth < 0) {
                    fprintf(stderr, "mkslug: invalid -d '%s'\n", val);
                    return -1;
                }
                break;
            }
            }
            *iref = i;
            return 0; /* value consumed the rest of this cluster */
        }
        /* boolean flags */
        switch (c) {
        case 'h': o->show_help = 1; break;
        case 'v': o->show_version = 1; break;
        case 'U': o->unslug = 1; break;
        case 'x': set_case(o, MS_CASE_NOCHANGE); break;
        case 'l': set_case(o, MS_CASE_LOWER); break;
        case 'u': set_case(o, MS_CASE_UPPER); break;
        case 't': set_case(o, MS_CASE_TITLE); break;
        case 'n': o->no_clipboard = 1; break;
        case 'f': o->force_file = 1; o->mode = MS_MODE_FILE; break;
        case 'r': o->do_rename = 1; break;
        case 'R': o->recurse = 1; break;
        default:
            fprintf(stderr, "mkslug: unknown option '-%c'\n", c);
            return -1;
        }
        p++;
    }
    *iref = i;
    return 0;
}

int ms_options_parse(int argc, char **argv, ms_options_t *o)
{
    int i;
    int no_more_opts = 0;
    int got_positional = 0;

    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (!no_more_opts && !strcmp(arg, "--")) {
            no_more_opts = 1;
            continue;
        }
        if (!no_more_opts && arg[0] == '-' && arg[1] != '\0') {
            int rc;
            if (arg[1] == '-')
                rc = parse_long(o, arg, &i, argc, argv);
            else
                rc = parse_short(o, arg, &i, argc, argv);
            if (rc != 0)
                return MS_EXIT_USAGE;
            /* short-circuit for help/version so a bad rest doesn't matter */
            if (o->show_help || o->show_version)
                return 0;
            continue;
        }
        /* positional argument */
        if (got_positional) {
            fprintf(stderr,
                    "mkslug: unexpected extra argument '%s' (only one "
                    "text/path is accepted)\n",
                    arg);
            return MS_EXIT_USAGE;
        }
        o->input = arg;
        o->have_input = 1;
        got_positional = 1;
    }

    /* -U (unslug) defaults to preserving case unless a case flag was given. */
    if (o->unslug && !o->casemode_set)
        o->casemode = MS_CASE_NOCHANGE;

    return 0;
}

void ms_print_version(void)
{
    printf("mkslug %s\n", MKSLUG_VERSION);
}

void ms_print_help(void)
{
    fputs(
"mkslug " MKSLUG_VERSION " - slugify and unslugify text, files, and directories\n"
"\n"
"Usage:\n"
"  mkslug [options] [<text> | <file-or-directory>]\n"
"\n"
"If no positional argument is given, text is read from standard input.\n"
"\n"
"Common options:\n"
"  -h, --help              Show this help and exit\n"
"  -v, --version           Show version information and exit\n"
"  -c, --config <file>     Override config file\n"
"                          (default: ~/.config/mkslug/default.conf, ~/.mkslug.conf)\n"
"  -U, --unslug            Unslug text/file/dir (preserves case by default)\n"
"\n"
"Case-conversion options (mutually exclusive; first one wins):\n"
"  -x, --no-change         Do not convert the case\n"
"  -l, --lower             Convert to lowercase (default in slug mode)\n"
"  -u, --upper             Convert to uppercase\n"
"  -t, --title             Convert to title case\n"
"\n"
"Text mode (positional argument treated as text):\n"
"  -n, --no-clipboard      Do not copy the result to the clipboard\n"
"                          (by default the slug is copied to the clipboard)\n"
"\n"
"File/directory mode (positional argument treated as a path):\n"
"  Unless -f is given, mkslug checks whether the path exists; if it does not,\n"
"  it falls back to text mode. File extensions are preserved, not slugified.\n"
"  -f, --force             Force file/dir mode even if the path does not exist\n"
"  -r, --rename            Rename the entry on disk to its slug\n"
"  -R, --recurse           Recurse into directories\n"
"  -X, --exclude <pat>     Comma-separated patterns to exclude in recursion\n"
"  -I, --include <pat>     Comma-separated patterns to include in recursion\n"
"  -d, --max-depth <n>     Maximum recursion depth (0 = unlimited; default 0)\n"
"\n"
"  In recursion with rename, files are renamed first, then directories from the\n"
"  bottom up, to avoid path-traversal races.\n"
"\n"
"Patterns (for -X / -I):\n"
"  glob     enclosed in '@'   e.g.  @*.tmp@,@*.exe@\n"
"  regex    enclosed in '/'   e.g.  /^.+\\.(doc|ppt|xls)x?$/\n"
"\n"
"Examples:\n"
"  mkslug \"Hello, World!\"            -> hello-world  (also copied to clipboard)\n"
"  mkslug -t \"my cool post\"          -> My-Cool-Post\n"
"  mkslug -U hello-world             -> hello world\n"
"  mkslug -r \"My Résumé.PDF\"         renames file to my-resume.PDF\n"
"  mkslug -Rr -X '@*.git@' ./docs    recursively slug-rename under ./docs\n"
"  echo \"Straße 1\" | mkslug          -> strasse-1\n",
        stdout);
}
