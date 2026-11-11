#!/usr/bin/env python3
"""Prepend a bare Doxygen @file so documented free functions are emitted.

Doxygen drops namespace-scope functions and enums unless the header itself
is a documented file. Classes are kept either way. Pack headers document the
functions and leave @file off, so the generated site shows only the pack
class. This filter supplies @file when the header already has a Doxygen
comment and no @file command. Undocumented headers are left alone.

A hand-written @file is kept, including its brief. The injected block has
no brief.
"""

import sys


def _comment_body(line):
    """Return the Doxygen command text of a comment line, or None."""
    s = line.strip()
    if s.startswith("/**") or s.startswith("/*!"):
        s = s[3:].strip()
    elif s.startswith("*"):
        s = s[1:].strip()
    elif s.startswith("///"):
        s = s[3:].strip()
    else:
        return None
    return s


def has_file_command(text):
    """True when the header already has a @file command, not a mention."""
    for line in text.splitlines():
        body = _comment_body(line)
        if body is None:
            continue
        if body.startswith("@file") or body.startswith("\\file"):
            rest = body[5:]
            if rest == "" or rest[0].isspace() or rest.startswith("*/"):
                return True
    return False


def has_doxygen_comment(text):
    return "/**" in text or "/*!" in text or "///" in text


def filtered_text(text):
    bom = ""
    if text.startswith("\ufeff"):
        bom = "\ufeff"
        text = text[1:]
    if has_doxygen_comment(text) and not has_file_command(text):
        nl = "\r\n" if "\r\n" in text else "\n"
        text = f"/**{nl} * @file{nl} */{nl}{nl}" + text
    return bom + text


def main():
    if len(sys.argv) != 2:
        sys.stderr.write("usage: doxygen_ensure_file.py <header>\n")
        return 2
    with open(sys.argv[1], "rb") as handle:
        raw = handle.read()
    text = filtered_text(raw.decode("utf-8"))
    sys.stdout.buffer.write(text.encode("utf-8"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
