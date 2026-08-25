/* config.c - load runtime settings from a simple key=value config file.
 *
 * Recognized keys (case-insensitive):
 *   separator      = -            single character used to join words
 *   case           = lower        nochange | lower | upper | title
 *   clipboard      = true         true/false/on/off/yes/no/1/0
 *   transliterate  = true         romanize non-ASCII via ICU
 *   collapse       = true         collapse runs of separators
 *
 * Lines beginning with '#' or ';' are comments; blank lines are ignored. */
#include "mkslug/config.h"
#include "mkslug/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ms_config_defaults(ms_config_t *cfg)
{
    cfg->separator = '-';
    cfg->clipboard = 1;
    cfg->transliterate = 1;
    cfg->default_case = MS_CASE_LOWER;
    cfg->collapse = 1;
}

static char *trim(char *s)
{
    char *end;
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
        s++;
    end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' ||
                       end[-1] == '\n'))
        *--end = '\0';
    return s;
}

static int parse_bool(const char *v, int *out)
{
    if (!ms_strcasecmp_ascii(v, "true") || !ms_strcasecmp_ascii(v, "on") ||
        !ms_strcasecmp_ascii(v, "yes") || !strcmp(v, "1")) {
        *out = 1;
        return 0;
    }
    if (!ms_strcasecmp_ascii(v, "false") || !ms_strcasecmp_ascii(v, "off") ||
        !ms_strcasecmp_ascii(v, "no") || !strcmp(v, "0")) {
        *out = 0;
        return 0;
    }
    return -1;
}

static int parse_case(const char *v, ms_case_t *out)
{
    if (!ms_strcasecmp_ascii(v, "nochange") || !ms_strcasecmp_ascii(v, "none"))
        *out = MS_CASE_NOCHANGE;
    else if (!ms_strcasecmp_ascii(v, "lower"))
        *out = MS_CASE_LOWER;
    else if (!ms_strcasecmp_ascii(v, "upper"))
        *out = MS_CASE_UPPER;
    else if (!ms_strcasecmp_ascii(v, "title"))
        *out = MS_CASE_TITLE;
    else
        return -1;
    return 0;
}

/* Parse an already-opened file. Returns 0 on success, -1 on syntax error. */
static int parse_stream(FILE *fp, const char *path, ms_config_t *cfg)
{
    char line[1024];
    int lineno = 0;

    while (fgets(line, sizeof line, fp)) {
        char *s = trim(line);
        char *eq, *key, *val;
        lineno++;
        if (*s == '\0' || *s == '#' || *s == ';')
            continue;
        eq = strchr(s, '=');
        if (!eq) {
            fprintf(stderr, "mkslug: %s:%d: expected key=value\n", path, lineno);
            return -1;
        }
        *eq = '\0';
        key = trim(s);
        val = trim(eq + 1);

        if (!ms_strcasecmp_ascii(key, "separator")) {
            if (val[0] == '\0') {
                fprintf(stderr, "mkslug: %s:%d: empty separator\n", path, lineno);
                return -1;
            }
            cfg->separator = val[0];
        } else if (!ms_strcasecmp_ascii(key, "case")) {
            if (parse_case(val, &cfg->default_case) != 0) {
                fprintf(stderr, "mkslug: %s:%d: invalid case '%s'\n", path,
                        lineno, val);
                return -1;
            }
        } else if (!ms_strcasecmp_ascii(key, "clipboard")) {
            if (parse_bool(val, &cfg->clipboard) != 0)
                goto badbool;
        } else if (!ms_strcasecmp_ascii(key, "transliterate")) {
            if (parse_bool(val, &cfg->transliterate) != 0)
                goto badbool;
        } else if (!ms_strcasecmp_ascii(key, "collapse")) {
            if (parse_bool(val, &cfg->collapse) != 0)
                goto badbool;
        } else {
            fprintf(stderr, "mkslug: %s:%d: unknown key '%s'\n", path, lineno,
                    key);
            return -1;
        }
        continue;
    badbool:
        fprintf(stderr, "mkslug: %s:%d: invalid boolean '%s'\n", path, lineno,
                val);
        return -1;
    }
    return 0;
}

static int load_file(const char *path, ms_config_t *cfg, int required)
{
    FILE *fp = fopen(path, "r");
    int rc;
    if (!fp) {
        if (required) {
            fprintf(stderr, "mkslug: cannot open config '%s'\n", path);
            return -1;
        }
        return 0; /* missing optional config is fine */
    }
    rc = parse_stream(fp, path, cfg);
    fclose(fp);
    return rc;
}

int ms_config_load(ms_config_t *cfg, const char *explicit_path)
{
    const char *home, *xdg;

    ms_config_defaults(cfg);

    if (explicit_path)
        return load_file(explicit_path, cfg, 1);

    xdg = getenv("XDG_CONFIG_HOME");
    home = getenv("HOME");

    if (xdg && *xdg) {
        char path[2048];
        snprintf(path, sizeof path, "%s/mkslug/default.conf", xdg);
        if (load_file(path, cfg, 0) != 0)
            return -1;
    } else if (home && *home) {
        char path[2048];
        snprintf(path, sizeof path, "%s/.config/mkslug/default.conf", home);
        if (load_file(path, cfg, 0) != 0)
            return -1;
    }

    if (home && *home) {
        char path[2048];
        snprintf(path, sizeof path, "%s/.mkslug.conf", home);
        if (load_file(path, cfg, 0) != 0)
            return -1;
    }
    return 0;
}
