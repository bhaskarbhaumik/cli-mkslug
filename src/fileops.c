/* fileops.c - slug operations on files and directories.
 *
 * Preview (default) prints "path -> new-name"; with -r the entry is renamed on
 * disk. With -R directories are traversed. To avoid path-traversal races, each
 * directory's file children are renamed first, then subdirectories bottom-up
 * (a directory is renamed only after everything beneath it has been handled).
 * File extensions are preserved verbatim and never slugified. */
#include "mkslug/fileops.h"
#include "mkslug/slug.h"
#include "mkslug/pattern.h"
#include "mkslug/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/stat.h>
#include <dirent.h>
#endif

/* ---- platform filesystem helpers --------------------------------------- */

int ms_path_exists(const char *path)
{
#if defined(_WIN32)
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return lstat(path, &st) == 0;
#endif
}

static int path_is_dir(const char *path)
{
#if defined(_WIN32)
    DWORD a = GetFileAttributesA(path);
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    if (lstat(path, &st) != 0)
        return 0;
    return S_ISDIR(st.st_mode);
#endif
}

/* Return 1 if both paths resolve to the same filesystem object. This is how a
 * case-only rename ("XYZ" -> "xyz") is recognized on case-insensitive volumes
 * (APFS, HFS+, NTFS): the destination "exists" but is really the source. */
static int same_file(const char *a, const char *b)
{
#if defined(_WIN32)
    BY_HANDLE_FILE_INFORMATION ia, ib;
    HANDLE ha, hb;
    int same = 0;
    ha = CreateFileA(a, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    hb = CreateFileA(b, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (ha != INVALID_HANDLE_VALUE && hb != INVALID_HANDLE_VALUE &&
        GetFileInformationByHandle(ha, &ia) &&
        GetFileInformationByHandle(hb, &ib)) {
        same = (ia.dwVolumeSerialNumber == ib.dwVolumeSerialNumber &&
                ia.nFileIndexHigh == ib.nFileIndexHigh &&
                ia.nFileIndexLow == ib.nFileIndexLow);
    }
    if (ha != INVALID_HANDLE_VALUE)
        CloseHandle(ha);
    if (hb != INVALID_HANDLE_VALUE)
        CloseHandle(hb);
    return same;
#else
    struct stat sa, sb;
    if (lstat(a, &sa) != 0 || lstat(b, &sb) != 0)
        return 0;
    return sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino;
#endif
}

/* A snapshot of a directory's entries. */
typedef struct {
    char **names;
    int *is_dir;
    int count;
    int cap;
} dirlist_t;

static void dirlist_init(dirlist_t *dl)
{
    dl->cap = 16;
    dl->count = 0;
    dl->names = ms_xmalloc((size_t)dl->cap * sizeof(char *));
    dl->is_dir = ms_xmalloc((size_t)dl->cap * sizeof(int));
}

static void dirlist_push(dirlist_t *dl, const char *name, int is_dir)
{
    if (dl->count == dl->cap) {
        dl->cap *= 2;
        dl->names = ms_xrealloc(dl->names, (size_t)dl->cap * sizeof(char *));
        dl->is_dir = ms_xrealloc(dl->is_dir, (size_t)dl->cap * sizeof(int));
    }
    dl->names[dl->count] = ms_xstrdup(name);
    dl->is_dir[dl->count] = is_dir;
    dl->count++;
}

static void dirlist_free(dirlist_t *dl)
{
    int i;
    for (i = 0; i < dl->count; i++)
        free(dl->names[i]);
    free(dl->names);
    free(dl->is_dir);
}

static char *join_path(const char *dir, const char *name)
{
    size_t dl = strlen(dir);
    size_t nl = strlen(name);
    int slash = (dl > 0 && dir[dl - 1] != '/' && dir[dl - 1] != '\\');
    char *out = ms_xmalloc(dl + (slash ? 1 : 0) + nl + 1);
    memcpy(out, dir, dl);
    if (slash)
        out[dl++] = '/';
    memcpy(out + dl, name, nl + 1);
    return out;
}

/* Read one directory's immediate children into `dl` (excluding "." / ".."). */
static int list_directory(const char *path, dirlist_t *dl)
{
    dirlist_init(dl);
#if defined(_WIN32)
    {
        WIN32_FIND_DATAA fd;
        HANDLE h;
        char *pat = join_path(path, "*");
        h = FindFirstFileA(pat, &fd);
        free(pat);
        if (h == INVALID_HANDLE_VALUE)
            return -1;
        do {
            if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
                continue;
            dirlist_push(dl, fd.cFileName,
                         (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
        return 0;
    }
#else
    {
        DIR *d = opendir(path);
        struct dirent *ent;
        if (!d)
            return -1;
        while ((ent = readdir(d)) != NULL) {
            char *full;
            int isd;
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;
            full = join_path(path, ent->d_name);
            isd = path_is_dir(full);
            free(full);
            dirlist_push(dl, ent->d_name, isd);
        }
        closedir(d);
        return 0;
    }
#endif
}

/* ---- filtering ---------------------------------------------------------- */

typedef struct {
    ms_patternset_t *include;
    ms_patternset_t *exclude;
} filters_t;

/* Whether an entry name should be acted upon (renamed). */
static int passes_filters(const filters_t *f, const char *name)
{
    if (f->exclude && ms_patternset_match(f->exclude, name))
        return 0;
    if (f->include && ms_patternset_count(f->include) > 0 &&
        !ms_patternset_match(f->include, name))
        return 0;
    return 1;
}

/* Whether we should descend into a subdirectory (exclude prunes traversal). */
static int may_descend(const filters_t *f, const char *name)
{
    if (f->exclude && ms_patternset_match(f->exclude, name))
        return 0;
    return 1;
}

/* ---- slug name computation --------------------------------------------- */

char *ms_slug_basename(const char *name, int is_dir, const ms_options_t *opt,
                       const ms_config_t *cfg)
{
    ms_case_t cm = opt->casemode;
    char *slug, *result;

    if (is_dir) {
        if (opt->unslug)
            slug = ms_unslugify(name, cm);
        else
            slug = ms_slugify(name, cfg->separator, cm, cfg->transliterate,
                              cfg->collapse);
        return slug; /* may be NULL on error */
    } else {
        size_t stem_len;
        const char *ext = ms_split_ext(name, &stem_len);
        char *stem = ms_xmalloc(stem_len + 1);
        size_t slen, elen;
        memcpy(stem, name, stem_len);
        stem[stem_len] = '\0';

        if (opt->unslug)
            slug = ms_unslugify(stem, cm);
        else
            slug = ms_slugify(stem, cfg->separator, cm, cfg->transliterate,
                              cfg->collapse);
        free(stem);
        if (!slug)
            return NULL;

        slen = strlen(slug);
        elen = strlen(ext);
        result = ms_xmalloc(slen + elen + 1);
        memcpy(result, slug, slen);
        memcpy(result + slen, ext, elen + 1);
        free(slug);
        return result;
    }
}

/* ---- rename / preview one entry ---------------------------------------- */

static int apply_entry(const char *dir, const char *name, int is_dir,
                       const ms_options_t *opt, const ms_config_t *cfg)
{
    char *newname = ms_slug_basename(name, is_dir, opt, cfg);
    char *oldpath, *newpath;
    int rc = MS_EXIT_OK;

    if (!newname) {
        fprintf(stderr, "mkslug: failed to compute slug for '%s'\n", name);
        return MS_EXIT_ERROR;
    }

    oldpath = join_path(dir, name);

    if (strcmp(name, newname) == 0) {
        if (!opt->do_rename)
            printf("%s (unchanged)\n", oldpath);
        free(newname);
        free(oldpath);
        return MS_EXIT_OK;
    }

    newpath = join_path(dir, newname);

    if (opt->do_rename) {
        /* Refuse to clobber a *different* existing entry, but allow a case-only
         * rename where the destination is the same object as the source (as on
         * case-insensitive filesystems, e.g. XYZ -> xyz on APFS/NTFS). */
        if (ms_path_exists(newpath) && !same_file(oldpath, newpath)) {
            fprintf(stderr,
                    "mkslug: refusing to overwrite existing '%s'\n", newpath);
            rc = MS_EXIT_ERROR;
        } else if (rename(oldpath, newpath) != 0) {
            fprintf(stderr, "mkslug: cannot rename '%s' -> '%s': %s\n", oldpath,
                    newpath, strerror(errno));
            rc = MS_EXIT_ERROR;
        } else {
            printf("renamed '%s' -> '%s'\n", oldpath, newname);
        }
    } else {
        printf("%s -> %s\n", oldpath, newname);
    }

    free(newname);
    free(oldpath);
    free(newpath);
    return rc;
}

/* ---- recursive traversal (post-order for directories) ------------------ */

static int process_dir(const char *dirpath, long depth, const ms_options_t *opt,
                       const ms_config_t *cfg, const filters_t *filters)
{
    dirlist_t dl;
    int i, rc = MS_EXIT_OK;

    if (list_directory(dirpath, &dl) != 0) {
        fprintf(stderr, "mkslug: cannot read directory '%s': %s\n", dirpath,
                strerror(errno));
        return MS_EXIT_ERROR;
    }

    /* Pass 1: files in this directory. */
    for (i = 0; i < dl.count; i++) {
        if (dl.is_dir[i])
            continue;
        if (!passes_filters(filters, dl.names[i]))
            continue;
        if (apply_entry(dirpath, dl.names[i], 0, opt, cfg) != MS_EXIT_OK)
            rc = MS_EXIT_ERROR;
    }

    /* Pass 2: subdirectories, depth-first, renamed bottom-up. */
    for (i = 0; i < dl.count; i++) {
        char *sub;
        int deeper;
        if (!dl.is_dir[i])
            continue;
        if (!may_descend(filters, dl.names[i]))
            continue;

        sub = join_path(dirpath, dl.names[i]);
        deeper = (opt->max_depth == 0) || (depth < opt->max_depth);
        if (deeper) {
            if (process_dir(sub, depth + 1, opt, cfg, filters) != MS_EXIT_OK)
                rc = MS_EXIT_ERROR;
        }
        /* Rename this subdirectory after its contents are handled. */
        if (passes_filters(filters, dl.names[i])) {
            if (apply_entry(dirpath, dl.names[i], 1, opt, cfg) != MS_EXIT_OK)
                rc = MS_EXIT_ERROR;
        }
        free(sub);
    }

    dirlist_free(&dl);
    return rc;
}

/* ---- entry point -------------------------------------------------------- */

/* Split a path into (dir prefix, base name). Trailing slashes are trimmed.
 * Both returned strings are malloc'd. */
static void split_path(const char *path, char **dir_out, char **base_out)
{
    size_t len = strlen(path);
    const char *slash;
    char *copy;

    while (len > 1 && (path[len - 1] == '/' || path[len - 1] == '\\'))
        len--;
    copy = ms_xmalloc(len + 1);
    memcpy(copy, path, len);
    copy[len] = '\0';

    slash = strrchr(copy, '/');
#if defined(_WIN32)
    {
        const char *bslash = strrchr(copy, '\\');
        if (bslash && (!slash || bslash > slash))
            slash = bslash;
    }
#endif
    if (slash) {
        size_t dlen = (size_t)(slash - copy);
        char *dir = ms_xmalloc(dlen + 1);
        memcpy(dir, copy, dlen);
        dir[dlen] = '\0';
        if (dlen == 0) { /* path like "/foo" */
            free(dir);
            dir = ms_xstrdup("/");
        }
        *dir_out = dir;
        *base_out = ms_xstrdup(slash + 1);
    } else {
        *dir_out = ms_xstrdup(".");
        *base_out = ms_xstrdup(copy);
    }
    free(copy);
}

int ms_fileops_run(const char *path, const ms_options_t *opt,
                   const ms_config_t *cfg)
{
    filters_t filters = {NULL, NULL};
    char *dir = NULL, *base = NULL;
    int is_dir, rc = MS_EXIT_OK;
    char *err = NULL;

    if (!opt->force_file && !ms_path_exists(path)) {
        fprintf(stderr, "mkslug: path '%s' does not exist\n", path);
        return MS_EXIT_NOTFOUND;
    }

    if (opt->exclude_pat) {
        filters.exclude = ms_patternset_compile(opt->exclude_pat, &err);
        if (!filters.exclude) {
            fprintf(stderr, "mkslug: --exclude: %s\n", err ? err : "invalid");
            free(err);
            return MS_EXIT_USAGE;
        }
    }
    if (opt->include_pat) {
        filters.include = ms_patternset_compile(opt->include_pat, &err);
        if (!filters.include) {
            fprintf(stderr, "mkslug: --include: %s\n", err ? err : "invalid");
            free(err);
            ms_patternset_free(filters.exclude);
            return MS_EXIT_USAGE;
        }
    }

    is_dir = path_is_dir(path);
    split_path(path, &dir, &base);

    if (is_dir && opt->recurse) {
        /* Process contents first, then optionally rename the top directory. */
        rc = process_dir(path, 1, opt, cfg, &filters);
        if (passes_filters(&filters, base)) {
            if (apply_entry(dir, base, 1, opt, cfg) != MS_EXIT_OK)
                rc = MS_EXIT_ERROR;
        }
    } else {
        if (passes_filters(&filters, base)) {
            if (apply_entry(dir, base, is_dir, opt, cfg) != MS_EXIT_OK)
                rc = MS_EXIT_ERROR;
        }
    }

    free(dir);
    free(base);
    ms_patternset_free(filters.include);
    ms_patternset_free(filters.exclude);
    return rc;
}
