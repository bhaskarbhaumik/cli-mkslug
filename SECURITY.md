# Security Policy

## Supported versions

Security fixes are released for the latest minor version only.

| Version | Supported |
|---------|-----------|
| 1.1.x   | ✅        |
| < 1.1   | ❌        |

## Reporting a vulnerability

**Please do not report security vulnerabilities in public issues, discussions,
or pull requests.**

Report them privately through GitHub's
[private vulnerability reporting](https://github.com/bhaskarbhaumik/cli-mkslug/security/advisories/new)
(*Security* tab → *Report a vulnerability*). Please include:

- the mkslug version (`mkslug --version`), OS, and compiler;
- the steps or input needed to reproduce the problem;
- what an attacker could gain from it, if you know.

### What to expect

- You will get an acknowledgement within **5 business days**.
- We will confirm the problem, agree on its severity, and work on a fix,
  keeping you updated along the way.
- Once a fix is released, we will publish a GitHub Security Advisory and credit
  you, unless you prefer to remain anonymous.

## Scope

mkslug renames files and calls external clipboard helpers, so we are
especially interested in:

- renames that escape the target directory, or follow symlinks unexpectedly;
- user text reaching a shell, or being interpreted as a command;
- memory-safety bugs (overflows, use-after-free) in parsing or slug generation;
- crashes or hangs triggered by crafted input, file names, or config files.

Vulnerabilities in ICU or PCRE2 should be reported to those projects directly.
