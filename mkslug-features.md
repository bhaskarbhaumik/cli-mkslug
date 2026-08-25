# Features of `mkslug`

A simple and fast CLI tool to slugify and unslugify text, perfect for creating branch names, URLs, and file names. It can also work on file/directories.


## Features
- **Slugify text**: Convert text to URL-friendly slugs (default behavior)
- **Unslugify text**: Convert slugs back to readable text
- **Clipboard integration**: Automatically copies results to clipboard
- **Fast and lightweight**: Built with C for maximum performance
- **Cross-platform**: Works on Windows, macOS, and Linux
- **Case conversion**: Output can be converted to all lowercase, all uppercase, or title case using `--lower`, `--upper`, or `--title`
- **Extended features**: See the Usage below


## Usage

```
mkslug [options] [<text> | <file-or-directory>]

  common options:
    -h | --help             Show detailed help information, usage, and examples
    -v | --version          Show version information
    -c | --config <file>    Override default configuration file (default config: ~/.mkslug.conf, ~/.config/mkslug/default.conf)
    -U | --unslug           Unslug text/file/dir (does not change case)

  case-conversion options:
    -x | --no-change        Do not convert the case
    -l | --lower            Convert to lowercase; (default)
    -u | --upper            Convert to uppercase
    -t | --title            Convert to title case
                            note: -l, -u -t are exclusive; first option considered

  <text>-mode options:
    The supplied positional argument is a text, which will be slugified and copied to clipboard.

    -n | --no-clipboard     .. (note: slug is copied to clipboard by default unless this option is mentioned)


  <file-or-directory>-mode options:
    The supplied positional argument is a file or directory, which will be slugified based on the other options.
    Unless '-f' option is supplied, checks for the existence of file/dir, reverts to <text> mode, if it does not exist.
    In this mode, file extensions are retained, and not slugified.

    -f | --force            Force file/directory mode (even if file/dir does not exist)
    -r | --rename           Renames the slug
    -R | --recurse          Recurse the action on all files, if directory (see notes below)
    -X | --exclude <pat>    List of comma-separated glob/regex patterns (see patterns below) to exclude in recursion
    -I | --include <pat>    List of comma-separated glob/regex patterns (see patterns below) to include in recursion
    -d | --max-depth        maximum-depth for recursion; (default: 0; means no limit)

    notes: in recursion '-R' with rename '-r', the files are renamed first; and then directories are renamed from
           bottom-to-top to avoid directory path traversal race

  glob/regex file/directory patterns:
    <pat>                   <pattern-1,pattern-2,...>
    glob-pattern            enclosed in '@' (example: @*.tmp@,@*.exe@)
    regex-pattern           enclosed in '/' (example: /^.+\.(doc|ppt|xls)x?$/); uses extended regular expressions

```
