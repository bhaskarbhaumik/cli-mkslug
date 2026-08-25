/* fileops.h - file and directory slug operations. */
#ifndef MKSLUG_FILEOPS_H
#define MKSLUG_FILEOPS_H

#include "mkslug/options.h"
#include "mkslug/config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Process a filesystem path according to the options and config.
 *
 * Without -r/--rename, the computed slug name(s) are printed (a preview of what
 * would change). With -r, entries are renamed on disk. With -R, directories are
 * traversed; files are renamed first and directories bottom-up to avoid path
 * traversal races. Include/exclude patterns filter recursion. File extensions
 * are preserved verbatim and not slugified.
 *
 * Returns MS_EXIT_OK on success, or an MS_EXIT_* error code. */
int ms_fileops_run(const char *path, const ms_options_t *opt,
                   const ms_config_t *cfg);

/* Compute the slugified form of a single base name, preserving the file
 * extension when `is_dir` is 0. Returns a malloc'd string (caller frees). */
char *ms_slug_basename(const char *name, int is_dir, const ms_options_t *opt,
                       const ms_config_t *cfg);

/* Return non-zero if `path` exists on the filesystem (does not follow the
 * final symlink). Used to decide auto text-vs-file mode. */
int ms_path_exists(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* MKSLUG_FILEOPS_H */
