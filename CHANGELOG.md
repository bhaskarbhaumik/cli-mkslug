# Changelog

All notable changes to this project are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.1.0] - 2026-10-14

### Added
- Config keys for every command-line option: `unslug`, `force`, `rename`,
  `recurse`, `max-depth`, `exclude` and `include` (alongside the existing
  `separator`, `case`, `clipboard`, `transliterate`, `collapse`). Keys accept
  `-` or `_` (`max-depth` / `max_depth`).
- `--no-unslug`, `--no-force`, `--no-rename` and `--no-recurse` to turn off a
  flag the config file turns on.

### Changed
- Recursion excludes `@.DS_Store@,@.git@` by default. `-X` replaces the
  configured list; `-X ''` excludes nothing.
- `config/mkslug.conf.example` now lists every key with its default value.
- CI: replaced the retired `macos-13` runner with `macos-15-intel`, moved to
  `actions/checkout@v5`, added job timeouts and read-only token permissions,
  and enabled Dependabot for GitHub Actions.

### Fixed
- Linux builds: `popen`, `pclose` and `lstat` were implicitly declared under
  `-std=c11` on glibc. Clang refused to compile, and GCC builds could crash when
  copying to the clipboard because `popen`'s pointer result was truncated.

### Documentation
- Added `AUTHORS.md`, `CODE_OF_CONDUCT.md` (Contributor Covenant 2.1),
  `CONTRIBUTING.md`, `SECURITY.md` and `VERSION.md`, plus issue and pull
  request templates.
- Project URLs now point at the GitHub repository.

## [1.0.0] - 2026-08-25

Initial public release ([`10d9ce0`](https://github.com/bhaskarbhaumik/cli-mkslug/commit/10d9ce0)).

### Added
- Slugify text into URL/branch/filename-friendly ASCII slugs.
- Unslugify (`-U`) slugs back into readable text.
- Unicode romanization via ICU (`café` → `cafe`, `Zürich` → `zurich`,
  `Слава` → `slava`).
- Case conversion: `--no-change`, `--lower`, `--upper`, `--title`.
- Clipboard integration: native Win32 API on Windows; `pbcopy` on macOS;
  `wl-copy`/`xclip`/`xsel` auto-detection on Linux.
- File/directory mode: preview and rename (`-r`), recursion (`-R`), depth limit
  (`-d`), and include/exclude filters (`-I`/`-X`) using glob (`@...@`) and
  PCRE2 regex (`/.../`) patterns. Extensions are preserved.
- Bottom-up directory renaming during recursion to avoid path-traversal races.
- Configuration file support (`~/.config/mkslug/default.conf`, `~/.mkslug.conf`,
  or `-c`).
- Three build systems: hand-written GNU Makefile, CMake, and Autotools.
- Man page, unit tests, and black-box CLI tests.
- CI matrix across GCC/Clang and Make/CMake for macOS, Linux, and Windows.

[Unreleased]: https://github.com/bhaskarbhaumik/cli-mkslug/compare/v1.1.0...HEAD
[1.1.0]: https://github.com/bhaskarbhaumik/cli-mkslug/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/bhaskarbhaumik/cli-mkslug/releases/tag/v1.0.0
