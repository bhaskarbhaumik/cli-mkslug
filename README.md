# mkslug

A fast, dependency-lean C command-line tool to **slugify** and **unslugify**
text — perfect for branch names, URLs, and filenames — that also operates on
files and directories.

```console
$ mkslug "Hello, World!"
hello-world                       # …and copied to your clipboard

$ mkslug -t "my cool post"
My-Cool-Post

$ mkslug -U hello-world
hello world

$ echo "Zürich, Straße 1" | mkslug
zurich-strasse-1
```

## Features

- **Slugify** any text into URL/branch/filename-safe ASCII.
- **Unslugify** (`-U`) slugs back into readable text.
- **Unicode-aware** — non-ASCII is romanized with ICU
  (`café` → `cafe`, `Zürich` → `zurich`, `Слава` → `slava`, `東京` → `dongjing`).
- **Clipboard integration** — results are copied automatically
  (native Win32 on Windows; `pbcopy` on macOS; `wl-copy`/`xclip`/`xsel` on Linux).
- **Case conversion** — `--no-change`, `--lower`, `--upper`, `--title`.
- **File & directory mode** — preview or rename on disk, recurse with
  include/exclude filters, preserve extensions, rename directories bottom-up.
- **Fast & lightweight** — written in portable C11.
- **Cross-platform** — macOS (Apple & Intel), Linux (amd64/arm64), Windows (amd64/arm64).

## Usage

```
mkslug [options] [<text> | <file-or-directory>]

Common options:
  -h, --help              Show detailed help and exit
  -v, --version           Show version information
  -c, --config <file>     Override config file
  -U, --unslug            Unslug (preserves case by default)

Case conversion (mutually exclusive; first wins):
  -x, --no-change         Do not convert case
  -l, --lower             Lowercase (default in slug mode)
  -u, --upper             Uppercase
  -t, --title             Title case

Text mode:
  -n, --no-clipboard      Do not copy the result to the clipboard

File/directory mode:
  -f, --force             Force file/dir mode even if the path does not exist
  -r, --rename            Rename the entry on disk
  -R, --recurse           Recurse into directories
  -X, --exclude <pat>     Comma-separated patterns to exclude
  -I, --include <pat>     Comma-separated patterns to include
  -d, --max-depth <n>     Max recursion depth (0 = unlimited)

Patterns:  glob in @...@   (e.g. @*.tmp@,@*.exe@)
           regex in /.../  (e.g. /^.+\.(doc|ppt|xls)x?$/)
           regex flags i/m/s/x may follow the closing slash (e.g. /^readme$/i)
```

If no positional argument is given, text is read from **standard input**.

See the [man page](man/mkslug.1) or run `mkslug --help` for full details.

## Building

mkslug depends on **[ICU](https://icu.unicode.org/)** (Unicode transliteration)
and **[PCRE2](https://www.pcre.org/)** (patterns). It builds with GCC or Clang
and supports three build systems.

### Install dependencies

| Platform | Command |
|----------|---------|
| macOS    | `brew install icu4c pcre2 pkg-config` |
| Debian/Ubuntu | `sudo apt-get install libicu-dev libpcre2-dev pkg-config build-essential` |
| Fedora   | `sudo dnf install libicu-devel pcre2-devel pkgconf-pkg-config` |
| Windows  | `vcpkg install icu pcre2` |

### GNU Make

```console
make                      # builds ./mkslug
make check                # runs the test suite
sudo make install         # installs to /usr/local
make CC=clang             # choose a compiler
make CC=gcc-16
```

### CMake

```console
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix /usr/local
```

### Autotools

Autotools builds **out of tree** (this protects the hand-written `./Makefile`):

```console
autoreconf --install
mkdir build && cd build
../configure
make
make check
sudo make install
```

## Configuration

Settings load from `~/.config/mkslug/default.conf` and `~/.mkslug.conf`
(or `-c <file>`). See [`config/mkslug.conf.example`](config/mkslug.conf.example):

```ini
separator     = -        # word separator
case          = lower    # nochange | lower | upper | title
clipboard     = true     # copy slug to clipboard
transliterate = true     # romanize non-ASCII via ICU
collapse      = true     # collapse repeated separators
```

## Examples

```console
# Rename a file, preserving the extension
$ mkslug -r "My Résumé.PDF"
renamed 'My Résumé.PDF' -> 'my-resume.PDF'

# Recursively slug-rename a tree, excluding VCS dirs
$ mkslug -Rr -X '@.git@,@node_modules@' ./docs

# Only touch office documents, up to 2 levels deep
$ mkslug -Rr -I '/^.+\.(docx?|pptx?|xlsx?)$/' -d 2 ./shared
```

## Project layout

```
include/mkslug/   public headers
src/              implementation (slug, pattern, clipboard, fileops, …)
test/             unit tests + black-box CLI tests
man/              troff man page
cmake/ m4/        build-system helpers
.github/          CI
```

## License

[MIT](LICENSE).
