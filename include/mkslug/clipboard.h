/* clipboard.h - cross-platform clipboard write. */
#ifndef MKSLUG_CLIPBOARD_H
#define MKSLUG_CLIPBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Place a UTF-8 string on the system clipboard.
 *
 * Backends:
 *   Windows  native Win32 clipboard (OpenClipboard/SetClipboardData, CF_UNICODETEXT)
 *   macOS    pbcopy
 *   Linux    wl-copy (Wayland), else xclip, else xsel (auto-detected)
 *
 * Returns 0 on success, -1 on failure. On failure `*err` (if non-NULL) is set
 * to a static human-readable reason. Clipboard support being unavailable is a
 * soft failure the caller may choose to warn about rather than abort. */
int ms_clipboard_set(const char *utf8, const char **err);

/* Whether a clipboard backend appears to be available in this environment. */
int ms_clipboard_available(void);

#ifdef __cplusplus
}
#endif

#endif /* MKSLUG_CLIPBOARD_H */
