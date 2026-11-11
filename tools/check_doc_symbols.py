#!/usr/bin/env python3
"""Fail if a documented namespace-scope function or enum is absent from Doxygen.

Classes show up without @file. Free functions and namespace enums do not, and
Doxygen emits no warning when it drops them. tools/doxygen_ensure_file.py
supplies the missing @file at generate time. This check reads the generated
XML and the headers that were the input, and fails if a documented symbol
did not survive.

Undocumented helpers are ignored. EXTRACT_ALL and EXTRACT_STATIC stay NO.
"""

import argparse
import os
import re
import sys
import xml.etree.ElementTree as ET

_SKIP_DIRS = {"third_party", ".git", "build"}
_SPECIFIERS = {
    "inline",
    "constexpr",
    "consteval",
    "constinit",
    "friend",
    "virtual",
    "explicit",
    "extern",
    "mutable",
    "typename",
    "typedef",
}
_MEMBER_KINDS = {"function", "enumeration", "enum"}


def _strip_comments(text):
    out = []
    i = 0
    n = len(text)
    while i < n:
        if text.startswith("//", i):
            end = text.find("\n", i)
            if end < 0:
                break
            out.append("\n")
            i = end + 1
            continue
        if text.startswith("/*", i):
            end = text.find("*/", i + 2)
            if end < 0:
                break
            out.append(" ")
            i = end + 2
            continue
        out.append(text[i])
        i += 1
    return "".join(out)


def _skip_string(text, i):
    quote = text[i]
    i += 1
    n = len(text)
    while i < n:
        if text[i] == "\\":
            i += 2
            continue
        if text[i] == quote:
            return i + 1
        i += 1
    return i


def _read_decl(text, i):
    """Read one statement. Return (text, index after terminator, '{' or ';')."""
    start = i
    n = len(text)
    paren = angle = bracket = 0
    while i < n:
        if text.startswith("//", i):
            end = text.find("\n", i)
            i = n if end < 0 else end + 1
            continue
        if text.startswith("/*", i):
            end = text.find("*/", i + 2)
            i = n if end < 0 else end + 2
            continue
        ch = text[i]
        if ch in "\"'":
            i = _skip_string(text, i)
            continue
        if ch == "(":
            paren += 1
        elif ch == ")" and paren:
            paren -= 1
        elif ch == "[":
            bracket += 1
        elif ch == "]" and bracket:
            bracket -= 1
        elif ch == "<" and paren == 0 and bracket == 0:
            angle += 1
        elif ch == ">" and angle and paren == 0 and bracket == 0:
            angle -= 1
        elif ch == "{" and paren == 0 and angle == 0 and bracket == 0:
            return text[start:i], i, "{"
        elif ch == ";" and paren == 0 and angle == 0 and bracket == 0:
            return text[start:i], i + 1, ";"
        i += 1
    return text[start:], n, ";"


def _matching_gt(text, i):
    """Index just after the '>' that matches a '<' at i. i points at '<'."""
    depth = 0
    n = len(text)
    while i < n:
        ch = text[i]
        if ch in "\"'":
            i = _skip_string(text, i)
            continue
        if ch == "<":
            depth += 1
        elif ch == ">":
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    return n


def _strip_prefix(decl):
    """Drop leading template heads, attributes, and decl-specifiers.

    Return (rest, is_static).
    """
    s = decl.strip()
    is_static = False
    while s:
        if s.startswith("[["):
            end = s.find("]]")
            if end < 0:
                break
            s = s[end + 2 :].strip()
            continue
        if s.startswith("template"):
            rest = s[len("template") :].lstrip()
            if rest.startswith("<"):
                s = rest[_matching_gt(rest, 0) :].strip()
                continue
        match = re.match(r"([A-Za-z_][\w]*)\b", s)
        if not match:
            break
        word = match.group(1)
        if word == "static":
            is_static = True
            s = s[match.end() :].strip()
            continue
        if word in _SPECIFIERS:
            s = s[match.end() :].strip()
            continue
        break
    return s, is_static


def _function_name(decl):
    """Identifier immediately before the parameter list, or None.

    Out-of-line definitions (Foo::bar()) return None. All-caps names are
    macro invocations and return None.
    """
    paren = angle = 0
    name_at = None
    i = 0
    n = len(decl)
    while i < n:
        ch = decl[i]
        if ch in "\"'":
            i = _skip_string(decl, i)
            continue
        if ch == "(" and angle == 0:
            if paren == 0:
                break
            paren += 1
        elif ch == ")" and paren:
            paren -= 1
        elif ch == "<" and paren == 0:
            angle += 1
        elif ch == ">" and angle and paren == 0:
            angle -= 1
        elif paren == 0 and (ch.isalpha() or ch == "_" or ch == "~"):
            prev = decl[i - 1] if i else ""
            if not (prev.isalnum() or prev == "_"):
                name_at = i
        i += 1
    if name_at is None or i >= n or decl[i] != "(":
        return None
    j = name_at
    while j < n and (decl[j].isalnum() or decl[j] == "_"):
        j += 1
    name = decl[name_at:j]
    if not name or name.isupper() or name in {
        "bool", "void", "int", "char", "float", "double", "auto", "const",
        "unsigned", "signed", "short", "long", "if", "for", "while", "switch",
        "return", "sizeof", "new", "delete", "typedef", "using",
    }:
        return None
    k = name_at - 1
    while k >= 0 and decl[k].isspace():
        k -= 1
    if k >= 0 and decl[k] == ":":
        return None
    return name


def _classify(decl):
    """Return ('function'|'enum'|None, name, is_static) for one statement."""
    raw = _strip_comments(decl).strip()
    if raw.startswith(("using ", "using\t", "typedef ", "typedef\t")):
        return None, None, False
    rest, is_static = _strip_prefix(raw)
    if rest.startswith("namespace"):
        return None, None, is_static
    if rest.startswith("enum"):
        match = re.match(r"enum(?:\s+(?:class|struct))?\s+([A-Za-z_][\w]*)", rest)
        if not match:
            return None, None, is_static
        return "enum", match.group(1), is_static
    if re.match(r"(class|struct|union)\b", rest):
        return None, None, is_static
    name = _function_name(rest)
    if not name:
        return None, None, is_static
    return "function", name, is_static


def _namespace_name(decl):
    s = _strip_comments(decl).strip()
    if re.match(r"namespace\s*$", s):
        return ""
    match = re.match(
        r"namespace\s+([A-Za-z_][\w]*(?:\s*::\s*[A-Za-z_][\w]*)*)\s*$", s
    )
    if not match:
        return ""
    return re.sub(r"\s+", "", match.group(1))


def _is_file_comment(body):
    for line in body.splitlines():
        s = line.strip().lstrip("*").strip()
        if s.startswith("@file") or s.startswith("\\file"):
            rest = s[5:]
            if rest == "" or rest[0].isspace() or rest.startswith("*/"):
                return True
    return False


def _expr_true(expr):
    """Whether Doxygen keeps a preprocessor branch.

    Unknown macros are 0, matching a PREDEFINED list of only RIGKIT_GLES=0.
    __cplusplus stays defined because Doxygen sets it for C++ headers.
    None means the expression is too odd to judge; the caller keeps the branch.
    """
    s = expr.strip()
    s = re.sub(
        r"defined\s*\(\s*([A-Za-z_]\w*)\s*\)|defined\s+([A-Za-z_]\w*)",
        lambda m: "1" if (m.group(1) or m.group(2)) == "__cplusplus" else "0",
        s,
    )

    def ident(match):
        name = match.group(0)
        if name in ("and", "or", "not"):
            return name
        if name == "__cplusplus":
            return "1"
        if name == "RIGKIT_GLES":
            return "0"
        return "0"

    s = re.sub(r"\b[A-Za-z_]\w*\b", ident, s)
    s = s.replace("&&", " and ").replace("||", " or ").replace("!", " not ")
    if not re.fullmatch(r"[0-9andort\s()]+", s):
        return None
    try:
        return bool(eval(s, {"__builtins__": {}}, {}))
    except Exception:
        return None


def _pp_line(text, i, line):
    parts = []
    n = len(text)
    while i < n and text[i] != "\n":
        if text[i] == "\\" and i + 1 < n and text[i + 1] == "\n":
            line += 1
            i += 2
            parts.append(" ")
            continue
        parts.append(text[i])
        i += 1
    return "".join(parts).strip(), i, line


def _pp_active(frames):
    return all(parent and cond for parent, cond in frames)


def _qual(stack):
    """Dotted namespace, '' at global scope, None inside an anonymous namespace."""
    parts = []
    for kind, name in stack:
        if kind != "ns":
            continue
        if name == "":
            return None
        parts.append(name)
    return "::".join(parts)


def _at_namespace(stack):
    return all(kind in ("ns", "extern") for kind, _name in stack)


def expected_symbols(text):
    """(line, qualified name) for documented namespace functions and enums."""
    found = []
    n = len(text)
    i = 0
    line = 1
    stack = []
    pending = None
    pp = []

    def consume_decl(i, line):
        nonlocal pending
        decl, nxt, term = _read_decl(text, i)
        line += decl.count("\n")
        raw = _strip_comments(decl).strip()
        if raw.startswith("namespace") and not raw.startswith("namespace_"):
            if term == "{":
                stack.append(("ns", _namespace_name(decl)))
                nxt += 1
            pending = None
            return nxt, line
        if raw.startswith("extern") and '"' in raw and term == "{":
            stack.append(("extern", None))
            nxt += 1
            pending = None
            return nxt, line
        kind, name, is_static = _classify(decl)
        if term == "{":
            if kind == "enum":
                stack.append(("type", name))
            elif re.match(r"(class|struct|union|enum)\b", _strip_prefix(raw)[0]):
                stack.append(("type", None))
            else:
                stack.append(("block", None))
            nxt += 1
        if (
            pending is not None
            and _at_namespace(stack[:-1] if term == "{" else stack)
            and kind in ("function", "enum")
            and name
            and not is_static
        ):
            qual = _qual(stack[:-1] if term == "{" and kind != "enum" else stack)
            # An enum's brace is a type scope; the enum itself lives outside it.
            if kind == "enum" and term == "{":
                qual = _qual(stack[:-1])
            if qual is not None:
                full = f"{qual}::{name}" if qual else name
                found.append((pending[0], full))
        pending = None
        return nxt, line

    while i < n:
        ch = text[i]
        if ch == "\n":
            line += 1
            i += 1
            continue
        if ch.isspace():
            i += 1
            continue
        if text.startswith("//", i):
            start_line = line
            if text.startswith("///", i) and _at_namespace(stack):
                parts = []
                while i < n and text.startswith("///", i):
                    end = text.find("\n", i)
                    if end < 0:
                        end = n
                    parts.append(text[i + 3 : end])
                    line += 1
                    i = end + 1
                    j = i
                    while j < n and text[j] in " \t":
                        j += 1
                    if text.startswith("///", j):
                        i = j
                    else:
                        break
                body = "\n".join(parts)
                if not _is_file_comment(body):
                    pending = (start_line, body)
            else:
                end = text.find("\n", i)
                if end < 0:
                    break
                line += 1
                i = end + 1
            continue
        if text.startswith("/*", i):
            end = text.find("*/", i + 2)
            if end < 0:
                break
            body = text[i + 2 : end]
            start_line = line
            line += body.count("\n")
            is_doc = text.startswith("/**", i) or text.startswith("/*!", i)
            is_trail = text.startswith("/**<", i) or text.startswith("/*!<", i)
            i = end + 2
            if is_doc and not is_trail and _at_namespace(stack) and not _is_file_comment(body):
                pending = (start_line, body)
            continue
        if ch == "#":
            directive, i, line = _pp_line(text, i, line)
            body = directive[1:].strip()
            parent = _pp_active(pp)
            if body.startswith("ifdef "):
                cond = _expr_true("defined(" + body[6:].strip() + ")")
                pp.append((parent, True if cond is None else cond))
            elif body.startswith("ifndef "):
                cond = _expr_true("!defined(" + body[7:].strip() + ")")
                pp.append((parent, True if cond is None else cond))
            elif body.startswith("if"):
                cond = _expr_true(body[2:])
                pp.append((parent, True if cond is None else cond))
            elif body.startswith("elif ") and pp:
                parent, cond = pp[-1]
                if cond:
                    pp[-1] = (parent, False)
                else:
                    nxt = _expr_true(body[5:])
                    pp[-1] = (parent, True if nxt is None else nxt)
            elif body.startswith("else") and pp:
                parent, cond = pp[-1]
                pp[-1] = (parent, not cond)
            elif body.startswith("endif") and pp:
                pp.pop()
            continue
        if not _pp_active(pp):
            end = text.find("\n", i)
            if end < 0:
                break
            i = end
            continue
        if ch in "\"'":
            i = _skip_string(text, i)
            continue
        if ch == "{":
            stack.append(("block", None))
            i += 1
            continue
        if ch == "}":
            if stack:
                stack.pop()
            i += 1
            continue
        i, line = consume_decl(i, line)
    return found


def iter_headers(root, exclude_examples):
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [
            name
            for name in dirnames
            if name not in _SKIP_DIRS and not name.startswith("build")
        ]
        if exclude_examples and "examples" in dirnames:
            dirnames.remove("examples")
        for name in filenames:
            if name.endswith((".h", ".hpp")):
                yield os.path.join(dirpath, name)


def load_xml_names(xml_dir):
    """Qualified function and enum names Doxygen actually emitted."""
    names = set()
    directory = os.path.abspath(xml_dir)
    if not os.path.isdir(directory):
        return names
    for filename in os.listdir(directory):
        if not filename.endswith(".xml") or filename == "index.xml":
            continue
        root = ET.parse(os.path.join(directory, filename)).getroot()
        for compound in root.iter("compounddef"):
            kind = compound.get("kind") or ""
            if kind not in ("namespace", "file"):
                continue
            prefix = ""
            if kind == "namespace":
                prefix = (compound.findtext("compoundname") or "").strip()
            for member in compound.findall("sectiondef/memberdef"):
                if (member.get("kind") or "") not in _MEMBER_KINDS:
                    continue
                if member.get("static") == "yes":
                    continue
                member_name = (member.findtext("name") or "").strip()
                if not member_name:
                    continue
                if kind == "namespace":
                    names.add(f"{prefix}::{member_name}" if prefix else member_name)
                    continue
                definition = (member.findtext("definition") or "").strip()
                match = re.search(r"([A-Za-z_][\w:]*)\s*$", definition)
                if match and "::" in match.group(1):
                    names.add(match.group(1))
                else:
                    names.add(member_name)
    return names


def collect(inputs, exclude_examples):
    expected = []

    def take(path):
        with open(path, encoding="utf-8", errors="replace") as handle:
            text = handle.read()
        for line, qual in expected_symbols(text):
            expected.append((path, line, qual))

    for root in inputs:
        if os.path.isfile(root):
            take(root)
        elif os.path.isdir(root):
            for path in iter_headers(root, exclude_examples):
                take(path)
    return expected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--xml", help="Doxygen XML output directory")
    parser.add_argument("--input", action="append", default=[], help="Header tree to scan")
    parser.add_argument(
        "--exclude-examples",
        action="store_true",
        help="Skip examples/, matching PackDoxyfile.in",
    )
    parser.add_argument("--dump", action="store_true", help="Print expected symbols and exit")
    args = parser.parse_args()
    if not args.input:
        parser.error("at least one --input is required")

    expected = collect(args.input, args.exclude_examples)
    if args.dump:
        for path, line, qual in expected:
            sys.stdout.write(f"{path}:{line} {qual}\n")
        sys.stdout.write(f"{len(expected)} symbols\n")
        return 0
    if not args.xml:
        parser.error("--xml is required")
    if not os.path.isdir(args.xml):
        sys.stderr.write(f"FAIL: no {args.xml} — regenerate docs\n")
        return 1

    present = load_xml_names(args.xml)
    missing = [(path, line, qual) for path, line, qual in expected if qual not in present]
    if missing:
        sys.stderr.write(
            f"FAIL: {len(missing)} documented free functions or namespace enums "
            "are absent from the Doxygen output:\n"
        )
        for path, line, qual in missing[:40]:
            sys.stderr.write(f"  {path}:{line} {qual}\n")
        if len(missing) > 40:
            sys.stderr.write(f"  … {len(missing) - 40} more\n")
        sys.stderr.write(
            "Namespace-scope functions are dropped when Doxygen never sees @file. "
            "The docs build prepends one (tools/doxygen_ensure_file.py). "
            "A static function is not emitted (EXTRACT_STATIC is NO).\n"
        )
        return 1
    sys.stdout.write(
        f"OK: {len(expected)} documented free functions and namespace enums are in the XML\n"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
