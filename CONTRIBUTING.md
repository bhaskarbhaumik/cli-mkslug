# Contributing to mkslug

Thanks for taking the time to contribute! Bug reports, fixes, documentation and
new ideas are all welcome. By participating you agree to follow the
[Code of Conduct](CODE_OF_CONDUCT.md).

## Ways to contribute

- **Report a bug** — open an issue using the *Bug report* template. Include
  your OS, compiler, `mkslug --version`, the exact command, and what you
  expected versus what happened.
- **Suggest a feature** — open an issue using the *Feature request* template
  before writing a large change, so we can agree on the design first.
- **Report a security problem** — do **not** open a public issue; follow
  [SECURITY.md](SECURITY.md).
- **Send a pull request** — see below.

## Development setup

mkslug is portable C11 with two dependencies: ICU and PCRE2.

| Platform      | Dependencies |
|---------------|--------------|
| macOS         | `brew install icu4c pcre2 pkg-config` |
| Debian/Ubuntu | `sudo apt-get install build-essential pkg-config libicu-dev libpcre2-dev` |
| Fedora        | `sudo dnf install gcc make pkgconf-pkg-config libicu-devel pcre2-devel` |
| Windows       | `vcpkg install icu pcre2` (build with CMake) |

Build and test with any of the three build systems:

```console
make && make check                                   # GNU make
cmake -S . -B build && cmake --build build \
  && ctest --test-dir build --output-on-failure      # CMake
autoreconf --install && mkdir _build && cd _build \
  && ../configure && make && make check              # Autotools (out of tree)
```

If you add a source file, add it to **all three** build systems
(`Makefile`, `CMakeLists.txt`, `Makefile.am`) and to `test/Makefile`.

## Coding guidelines

- **C11, no compiler extensions.** The code must build warning-free with GCC
  and Clang (`-Wall -Wextra -Wshadow ...`) and with MSVC.
- **POSIX APIs:** files that call POSIX functions (`popen`, `lstat`,
  `opendir`, ...) define `_POSIX_C_SOURCE 200809L` before any include, because
  `-std=c11` hides them on glibc. Wrap platform-specific code in
  `#if defined(_WIN32)`.
- **Formatting:** run `make format` (uses `.clang-format`) before committing.
- **Never pass user text through a shell.** Clipboard data goes through stdin.
- **Memory:** use the `ms_x*` allocation helpers in `src/util.c`; every owned
  string has a matching `*_free` function.
- **Keep the CLI, config and docs in sync.** A new option normally needs:
  the parser (`src/options.c`), a config key (`src/config.c`,
  `config/mkslug.conf.example`), `--help`, the man page (`man/mkslug.1`), the
  README, and a test.

## Tests

- Unit tests live in `test/test_*.c`; black-box CLI tests in `test/cli_test.sh`.
- Every bug fix should come with a test that fails without the fix.
- CLI tests must pass `-n` or a `-c` config file so they never touch the real
  clipboard or the developer's own `~/.config/mkslug/default.conf`.

## Commits and pull requests

- Use [Conventional Commits](https://www.conventionalcommits.org/):
  `feat:`, `fix:`, `docs:`, `test:`, `build:`, `ci:`, `refactor:`, `chore:`.
- Keep each pull request focused on one change, and make sure CI passes on all
  platforms.
- Add an entry under **[Unreleased]** in [CHANGELOG.md](CHANGELOG.md) for any
  change users will notice.
- Do not bump the version in a feature PR; maintainers do that when they cut a
  release (see [VERSION.md](VERSION.md)).

## License

By contributing, you agree that your contributions are licensed under the
[MIT License](LICENSE).
