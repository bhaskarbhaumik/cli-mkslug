# Changelog

All notable changes to this project are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-07-26

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

[1.0.0]: https://example.org/mkslug/releases/tag/v1.0.0
