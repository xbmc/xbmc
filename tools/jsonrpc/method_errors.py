#!/usr/bin/env python3
#
#  Copyright (C) 2026 Team Kodi
#  This file is part of Kodi - https://kodi.tv
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#  See LICENSES/README.md for more information.
#
"""Derive which errors each JSON-RPC method can return from its handler.

The errors a method can return are the JSONRPC_STATUS names its handler's
body mentions, plus those of every JSONRPC_STATUS function it calls,
transitively.  This module computes that closure from the source text in
xbmc/interfaces/json-rpc and compares it with the "errors" member each
method declares in methods.json.

    python tools/jsonrpc/method_errors.py            # report any drift
    python tools/jsonrpc/method_errors.py --write    # declare the derived sets
    python tools/jsonrpc/method_errors.py --explain Player.Open

The derived set is a superset of the truth: a status named in a comparison
counts as if it were returned.  It is never a subset, which is the property
a client generated from the declarations depends on.
"""

import json
import re
import sys
from collections import defaultdict
from pathlib import Path

import kodi_schema

SOURCE_DIR = kodi_schema.SCHEMA_DIR.parent
METHOD_MAP = SOURCE_DIR / "JSONServiceDescription.cpp"

# Produced before a handler runs, so every method can return them
PRE_DISPATCH = ("InvalidRequest", "MethodNotFound", "InvalidParams",
                "ParseError", "BadPermission")
SUCCESS = ("OK", "ACK")

_DEFINITION = re.compile(
    r"^[ 	]*(?:static\s+)?JSONRPC_STATUS\s+((?:JSONRPC::)?(?:\w+::)?\w+)\s*\(",
    re.MULTILINE)
# A call written without an object: a static of the same class, a qualified
# static, or a free function; obj.f( and ptr->f( are excluded.
_CALL = re.compile(r"(?<![\w.])(?<!->)((?:\w+::)?\w+)\s*\(")
_MAP_ENTRY = re.compile(r'\{\s*"([\w.]+)"\s*,\s*(\w+::\w+)\s*\}')


def _strip(text):
    """Blank comments and string literals so their contents are not parsed."""
    out = []
    i, n = 0, len(text)
    while i < n:
        if text.startswith("//", i):
            end = text.find("\n", i)
            i = n if end < 0 else end
        elif text.startswith("/*", i):
            end = text.find("*/", i + 2)
            end = n if end < 0 else end + 2
            out.append("\n" * text.count("\n", i, end))
            i = end
        elif text[i] in "\"'":
            quote = text[i]
            i += 1
            while i < n and text[i] != quote:
                i += 2 if text[i] == "\\" else 1
            out.append(quote + quote)
            i += 1
        else:
            out.append(text[i])
            i += 1
    return "".join(out)


def _matching(text, start, open_char, close_char):
    depth = 0
    for i in range(start, len(text)):
        if text[i] == open_char:
            depth += 1
        elif text[i] == close_char:
            depth -= 1
            if depth == 0:
                return i
    raise ValueError(f"unbalanced {open_char} at offset {start}")


def _definitions(text):
    """Yield (qualified name, body) for each JSONRPC_STATUS function defined."""
    for match in _DEFINITION.finditer(text):
        params_end = _matching(text, match.end() - 1, "(", ")")
        body_start = params_end + 1
        while text[body_start] not in "{;":
            body_start += 1
        if text[body_start] == ";":
            continue
        body_end = _matching(text, body_start, "{", "}")
        name = match.group(1).removeprefix("JSONRPC::")
        yield name, text[body_start:body_end + 1]


def _status_names(taxonomy):
    return [error["name"] for error in taxonomy]


def build_graph(source_dir=SOURCE_DIR, taxonomy=None):
    """Return (statuses, calls) per JSONRPC_STATUS function in source_dir."""
    taxonomy = taxonomy or kodi_schema.load_error_taxonomy()
    status_pattern = re.compile(
        r"\b(" + "|".join(_status_names(taxonomy) + list(SUCCESS)) + r")\b")
    bodies = {}
    for pattern in ("*.h", "*.cpp"):
        for path in sorted(Path(source_dir).glob(pattern)):
            text = _strip(path.read_text(encoding="utf-8"))
            bodies.update(_definitions(text))
    by_bare_name = defaultdict(set)
    for name in bodies:
        by_bare_name[name.rsplit("::", 1)[-1]].add(name)

    statuses = {}
    calls = {}
    for name, body in bodies.items():
        statuses[name] = set(status_pattern.findall(body)) - set(SUCCESS)
        cls = name.split("::")[0] if "::" in name else None
        callees = set()
        for callee in _CALL.findall(body):
            if "::" in callee:
                if callee in bodies:
                    callees.add(callee)
            else:
                candidates = by_bare_name.get(callee, set())
                same_class = {c for c in candidates
                              if cls and c.startswith(cls + "::")}
                free = {c for c in candidates if "::" not in c}
                # a bare call that neither the class nor a free function defines reaches a
                # static inherited from the one class that does
                inherited = {c for c in candidates if "::" in c}
                callees |= same_class or free or (inherited if len(inherited) == 1 else set())
        callees.discard(name)
        calls[name] = callees
    return statuses, calls


def method_handlers(method_map=METHOD_MAP):
    """Return {method name: handler function} from the C++ method table."""
    text = Path(method_map).read_text(encoding="utf-8")
    return dict(_MAP_ENTRY.findall(text))


def derive(source_dir=SOURCE_DIR, taxonomy=None):
    """Return {method name: [error names]} in taxonomy order."""
    taxonomy = taxonomy or kodi_schema.load_error_taxonomy()
    order = {name: index for index, name in enumerate(_status_names(taxonomy))}
    statuses, calls = build_graph(source_dir, taxonomy)

    closure = {name: set(found) for name, found in statuses.items()}
    changed = True
    while changed:
        changed = False
        for name, callees in calls.items():
            merged = closure[name].union(*(closure[c] for c in callees))
            if merged != closure[name]:
                closure[name] = merged
                changed = True

    handlers = method_handlers(Path(source_dir) / METHOD_MAP.name)
    derived = {}
    for method, handler in handlers.items():
        if handler not in closure:
            raise ValueError(f"{method} maps to {handler}, which is not defined")
        derived[method] = sorted(closure[handler], key=order.__getitem__)
    return derived


def explain(method, source_dir=SOURCE_DIR):
    """Print the call tree behind a method's derived error set."""
    statuses, calls = build_graph(source_dir)
    handler = method_handlers(Path(source_dir) / METHOD_MAP.name)[method]

    def walk(name, depth, seen):
        print("  " * depth + name, sorted(statuses[name]))
        for callee in sorted(calls[name]):
            if callee not in seen:
                seen.add(callee)
                walk(callee, depth + 1, seen)

    walk(handler, 0, {handler})


def declared(schema_dir=kodi_schema.SCHEMA_DIR):
    methods = kodi_schema.load_service(schema_dir)["methods"]
    return {name: method.get("errors") for name, method in methods.items()}


def write(schema_dir=kodi_schema.SCHEMA_DIR, source_dir=SOURCE_DIR):
    """Declare the derived error set on every method in methods.json."""
    path = Path(schema_dir) / "methods.json"
    methods = json.loads(path.read_text(encoding="utf-8"))
    derived = derive(source_dir)
    for name, method in methods.items():
        method.pop("errors", None)
        entries = list(method.items())
        after = next((i for i, (key, _) in enumerate(entries)
                      if key == "returns"), len(entries) - 1)
        entries.insert(after + 1, ("errors", derived[name]))
        methods[name] = dict(entries)
    path.write_text(json.dumps(methods, indent=2, ensure_ascii=False) + "\n",
                    encoding="utf-8")


def main(argv):
    if len(argv) == 2 and argv[0] == "--explain":
        explain(argv[1])
        return 0
    if argv == ["--write"]:
        write()
        return 0
    if argv:
        print(__doc__, file=sys.stderr)
        return 2
    schema = declared()
    drift = {name: errors for name, errors in derive().items()
             if schema.get(name) != errors}
    if not drift:
        print("every method declares the errors its handler can return")
        return 0
    for name, errors in drift.items():
        print(f"{name}: declares {schema.get(name)}, handler returns {errors}")
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
