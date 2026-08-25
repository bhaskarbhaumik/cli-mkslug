/* main.c - mkslug entry point.
 *
 * Orchestration:
 *   1. parse options            (options.c)
 *   2. load configuration       (config.c)
 *   3. resolve text vs file mode
 *   4. text mode  -> slug/unslug the string, print, copy to clipboard
 *      file mode  -> rename/preview paths (fileops.c)
 */
#include "mkslug/mkslug.h"
#include "mkslug/options.h"
#include "mkslug/config.h"
#include "mkslug/slug.h"
#include "mkslug/clipboard.h"
#include "mkslug/fileops.h"
#include "mkslug/util.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int run_text_mode(const char *text, const ms_options_t *opt,
                         const ms_config_t *cfg)
{
    char *result;
    int rc = MS_EXIT_OK;

    if (opt->unslug)
        result = ms_unslugify(text, opt->casemode);
    else
        result = ms_slugify(text, cfg->separator, opt->casemode,
                            cfg->transliterate, cfg->collapse);

    if (!result) {
        fprintf(stderr, "mkslug: failed to process input (invalid UTF-8?)\n");
        return MS_EXIT_ERROR;
    }

    printf("%s\n", result);

    /* Copy to clipboard unless disabled by flag or config. */
    if (!opt->no_clipboard && cfg->clipboard) {
        const char *err = NULL;
        if (ms_clipboard_set(result, &err) != 0) {
            fprintf(stderr, "mkslug: clipboard unavailable: %s\n",
                    err ? err : "unknown error");
            /* Non-fatal: the result is still printed to stdout. */
        }
    }

    free(result);
    return rc;
}

int main(int argc, char **argv)
{
    ms_options_t opt;
    ms_config_t cfg;
    int rc = MS_EXIT_OK;
    char *stdin_text = NULL;
    const char *text;

    setlocale(LC_ALL, "");

    ms_options_init(&opt);
    if ((rc = ms_options_parse(argc, argv, &opt)) != 0) {
        ms_options_free(&opt);
        return rc;
    }

    if (opt.show_help) {
        ms_print_help();
        ms_options_free(&opt);
        return MS_EXIT_OK;
    }
    if (opt.show_version) {
        ms_print_version();
        ms_options_free(&opt);
        return MS_EXIT_OK;
    }

    if (ms_config_load(&cfg, opt.config_path) != 0) {
        ms_options_free(&opt);
        return MS_EXIT_ERROR;
    }

    /* If the user did not set a case flag and we are slugifying, fall back to
     * the configured default case. (Unslug already defaulted to no-change.) */
    if (!opt.casemode_set && !opt.unslug)
        opt.casemode = cfg.default_case;

    /* Decide file vs text mode. */
    if (opt.force_file) {
        opt.mode = MS_MODE_FILE;
    } else if (opt.have_input && ms_path_exists(opt.input)) {
        opt.mode = MS_MODE_FILE;
    } else {
        opt.mode = MS_MODE_TEXT;
    }

    if (opt.mode == MS_MODE_FILE) {
        if (!opt.have_input) {
            fprintf(stderr, "mkslug: --force requires a path argument\n");
            ms_options_free(&opt);
            return MS_EXIT_USAGE;
        }
        rc = ms_fileops_run(opt.input, &opt, &cfg);
        ms_options_free(&opt);
        return rc;
    }

    /* Text mode: use the positional argument, else read stdin. */
    if (opt.have_input) {
        text = opt.input;
    } else {
        size_t n;
        stdin_text = ms_read_all_stdin(&n);
        ms_chomp(stdin_text);
        text = stdin_text;
    }

    rc = run_text_mode(text, &opt, &cfg);

    free(stdin_text);
    ms_options_free(&opt);
    return rc;
}
