# mkslug

[![CI](https://github.com/bhaskarbhaumik/cli-mkslug/actions/workflows/ci.yml/badge.svg)](https://github.com/bhaskarbhaumik/cli-mkslug/actions/workflows/ci.yml)
[![Version](https://img.shields.io/github/v/tag/bhaskarbhaumik/cli-mkslug?label=version&sort=semver)](CHANGELOG.md)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![C11](https://img.shields.io/badge/C-11-informational)

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
      --no-unslug         Slugify even if the config sets unslug = true

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
                          (default: @.DS_Store@,@.git@; -X '' excludes nothing)
  -I, --include <pat>     Comma-separated patterns to include
  -d, --max-depth <n>     Max recursion depth (0 = unlimited)
      --no-force, --no-rename, --no-recurse
                          Turn off a flag enabled in the config file

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
(or `-c <file>`). Every command-line option except `-h`, `-v` and `-c` has a
config key; a flag given on the command line always wins, and `-X`/`-I` replace
the configured pattern list (`-X ''` clears it). Flags that a config can turn on
have `--no-*` counterparts (`--no-unslug`, `--no-force`, `--no-rename`,
`--no-recurse`). See [`config/mkslug.conf.example`](config/mkslug.conf.example);
the values below are the built-in defaults:

```ini
separator     = -        # word separator
case          = lower    # nochange | lower | upper | title   (-x/-l/-u/-t)
transliterate = true     # romanize non-ASCII via ICU
collapse      = true     # collapse repeated separators
unslug        = false    # unslugify instead of slugify        (-U)
clipboard     = true     # copy slug to clipboard              (-n disables)
force         = false    # force file/dir mode                 (-f)
rename        = false    # rename on disk, else preview        (-r)
recurse       = false    # recurse into directories            (-R)
max-depth     = 0        # recursion depth, 0 = unlimited      (-d)
exclude       = @.DS_Store@, @.git@   # patterns to skip      (-X)
include       =          # patterns to keep; empty = all       (-I)
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
config/           example configuration file
scripts/          developer helper scripts
.github/          CI, Dependabot, issue and pull request templates
```

## Contributing

Contributions are welcome. Read [CONTRIBUTING.md](CONTRIBUTING.md) and the
[Code of Conduct](CODE_OF_CONDUCT.md) first, and report security issues as
described in [SECURITY.md](SECURITY.md). Release notes live in
[CHANGELOG.md](CHANGELOG.md), and the versioning policy in
[VERSION.md](VERSION.md). See [AUTHORS.md](AUTHORS.md) for who wrote mkslug.

## License

[MIT](LICENSE) © Bhaskar Bhaumik and the mkslug contributors.
