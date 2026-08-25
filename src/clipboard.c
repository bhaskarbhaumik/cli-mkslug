/* clipboard.c - cross-platform clipboard write.
 *
 *   Windows : native Win32 clipboard API (CF_UNICODETEXT)
 *   macOS   : pbcopy
 *   Linux   : wl-copy (Wayland) -> xclip -> xsel (first available)
 *
 * The payload always travels via stdin (POSIX) or a UTF-16 memory object
 * (Windows), so user text is never interpolated into a shell command. */
#include "mkslug/clipboard.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)

#include <windows.h>

int ms_clipboard_available(void)
{
    return 1;
}

int ms_clipboard_set(const char *utf8, const char **err)
{
    int wlen;
    HGLOBAL hmem;
    LPWSTR dst;

    if (!utf8) {
        if (err) *err = "null text";
        return -1;
    }
    wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (wlen <= 0) {
        if (err) *err = "utf-8 to utf-16 conversion failed";
        return -1;
    }
    if (!OpenClipboard(NULL)) {
        if (err) *err = "OpenClipboard failed";
        return -1;
    }
    EmptyClipboard();
    hmem = GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)wlen * sizeof(WCHAR));
    if (!hmem) {
        CloseClipboard();
        if (err) *err = "GlobalAlloc failed";
        return -1;
    }
    dst = (LPWSTR)GlobalLock(hmem);
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, dst, wlen);
    GlobalUnlock(hmem);
    if (!SetClipboardData(CF_UNICODETEXT, hmem)) {
        GlobalFree(hmem);
        CloseClipboard();
        if (err) *err = "SetClipboardData failed";
        return -1;
    }
    CloseClipboard();
    return 0;
}

#else /* POSIX */

#include <unistd.h>

/* Return 1 if `name` is an executable found on PATH. */
static int on_path(const char *name)
{
    const char *path = getenv("PATH");
    char buf[4096];
    const char *p;

    if (!path)
        path = "/usr/bin:/bin:/usr/local/bin";
    p = path;
    while (*p) {
        const char *colon = strchr(p, ':');
        size_t dlen = colon ? (size_t)(colon - p) : strlen(p);
        if (dlen > 0 && dlen + 1 + strlen(name) + 1 < sizeof buf) {
            memcpy(buf, p, dlen);
            buf[dlen] = '/';
            strcpy(buf + dlen + 1, name);
            if (access(buf, X_OK) == 0)
                return 1;
        }
        if (!colon)
            break;
        p = colon + 1;
    }
    return 0;
}

/* Pick the clipboard command line for this environment, or NULL. */
static const char *choose_command(void)
{
#if defined(__APPLE__)
    if (on_path("pbcopy"))
        return "pbcopy";
    return NULL;
#else
    if (getenv("WAYLAND_DISPLAY") && on_path("wl-copy"))
        return "wl-copy";
    if (on_path("xclip"))
        return "xclip -selection clipboard -in";
    if (on_path("xsel"))
        return "xsel --clipboard --input";
    /* Fall back even without $WAYLAND_DISPLAY, in case it is set later. */
    if (on_path("wl-copy"))
        return "wl-copy";
    return NULL;
#endif
}

int ms_clipboard_available(void)
{
    return choose_command() != NULL;
}

int ms_clipboard_set(const char *utf8, const char **err)
{
    const char *cmd = choose_command();
    FILE *fp;
    size_t len, wrote;
    int rc;

    if (!utf8) {
        if (err) *err = "null text";
        return -1;
    }
    if (!cmd) {
        if (err) {
#if defined(__APPLE__)
            *err = "pbcopy not found";
#else
            *err = "no clipboard tool found (install wl-clipboard, xclip, or xsel)";
#endif
        }
        return -1;
    }
    fp = popen(cmd, "w");
    if (!fp) {
        if (err) *err = "failed to launch clipboard helper";
        return -1;
    }
    len = strlen(utf8);
    wrote = fwrite(utf8, 1, len, fp);
    rc = pclose(fp);
    if (wrote != len) {
        if (err) *err = "short write to clipboard helper";
        return -1;
    }
    if (rc != 0) {
        if (err) *err = "clipboard helper exited with an error";
        return -1;
    }
    return 0;
}

#endif /* platform */
