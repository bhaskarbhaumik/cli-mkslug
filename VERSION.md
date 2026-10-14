# Versioning

**Current version: 1.1.0** (2026-10-14). See [CHANGELOG.md](CHANGELOG.md)
for what changed in each release.

mkslug follows [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html):

| Part      | Bumped when                                                     |
|-----------|-----------------------------------------------------------------|
| **MAJOR** | a CLI option, config key, or output format changes incompatibly |
| **MINOR** | features are added in a backward-compatible way                 |
| **PATCH** | bugs are fixed with no change to the interface                  |

The interface covered by this promise is the command-line options, the config
file keys and their values, the exit codes, and the text printed to stdout.
Diagnostics printed to stderr may change in any release.

## Where the version lives

The plain-text [`VERSION`](VERSION) file is the single source of truth. All
three build systems read it:

- **GNU make** passes `-DMKSLUG_VERSION="$(VERSION)"` to the compiler.
- **CMake** reads it into `project(VERSION ...)` and generates
  `include/mkslug/version.h` from `version.h.in`.
- **Autotools** reads it in `AC_INIT` from `configure.ac`.

Two other files repeat the version and must be updated with it:

- `include/mkslug/mkslug.h` — the `MKSLUG_VERSION` fallback used when no build
  system defines it;
- `man/mkslug.1` — the `.TH` line (version and date).

## Release checklist

1. Update `VERSION`, the fallback in `include/mkslug/mkslug.h`, the `.TH` line
   in `man/mkslug.1`, and **Current version** at the top of this file.
2. In `CHANGELOG.md`, rename **[Unreleased]** to `[X.Y.Z] - YYYY-MM-DD`, add a
   new empty **[Unreleased]** section, and update the compare links at the
   bottom.
3. Run `make check`, and check that `./mkslug --version` prints the new version.
4. Commit: `chore(release): vX.Y.Z`.
5. Tag and push:
   ```console
   git tag -a vX.Y.Z -m "mkslug X.Y.Z"
   git push origin main vX.Y.Z
   ```
6. Optionally create a GitHub release from the tag, using the changelog section
   as the release notes.

## Release history

| Version | Date       | Tag                                                                         |
|---------|------------|-----------------------------------------------------------------------------|
| 1.1.0   | 2026-10-14 | [`v1.1.0`](https://github.com/bhaskarbhaumik/cli-mkslug/releases/tag/v1.1.0) |
| 1.0.0   | 2026-08-25 | [`v1.0.0`](https://github.com/bhaskarbhaumik/cli-mkslug/releases/tag/v1.0.0) |
