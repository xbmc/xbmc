#!/usr/bin/env python3
#
#  Copyright (C) 2026 Team Kodi
#  This file is part of Kodi - https://kodi.tv
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#  See LICENSES/README.md for more information.
#
"""Shared loader for the JSON-RPC service description.

Reads the schema corpus (methods.json, types.json, notifications.json,
version.txt) and the error taxonomy from JSONRPCUtils.h into plain Python
structures that the OpenRPC and AsyncAPI generators consume.  Source key
order is preserved everywhere, so running a generator twice yields
byte-identical files.

The enum types registered at runtime from C++ tables have no definition in
types.json; placeholder schemas flagged with "x-kodi-runtime-enum" are
synthesized for them.
"""

import json
import re
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
SCHEMA_DIR = REPO_ROOT / "xbmc" / "interfaces" / "json-rpc" / "schema"
UTILS_HEADER = (REPO_ROOT / "xbmc" / "interfaces" / "json-rpc"
                / "JSONRPCUtils.h")

# The enum types CJSONRPC::Initialize registers from C++ tables; the only
# names a $ref may point at without a definition in types.json.
RUNTIME_ENUM_TYPES = frozenset({
    "Addon.Types",
    "GUI.Window",
    "Input.Action",
    "List.Filter.Fields.Albums",
    "List.Filter.Fields.Artists",
    "List.Filter.Fields.Episodes",
    "List.Filter.Fields.Movies",
    "List.Filter.Fields.MusicVideos",
    "List.Filter.Fields.Songs",
    "List.Filter.Fields.TVShows",
    "List.Filter.Fields.Textures",
    "List.Filter.Operators",
})

RUNTIME_ENUM_PLACEHOLDER = {
    "type": "string",
    "description": ("Enumerated at runtime by the running Kodi instance; "
                    "call JSONRPC.Introspect for the live value list."),
    "x-kodi-runtime-enum": True,
}

_STRING_LITERALS = r'(?:"(?:[^"\\]|\\.)*"\s*)+'

_DESCRIPTION_ENTRY = re.compile(
    r"\{\s*(\w+)\s*,\s*"
    r"(" + _STRING_LITERALS + r")\s*,\s*"
    r"(" + _STRING_LITERALS + r")\s*,\s*"
    r"(" + _STRING_LITERALS + r")\s*,\s*"
    r"(true|false)\s*,?\s*\}",
    re.DOTALL)

_ENUM_ENTRY = re.compile(r"^\s*(\w+)\s*=\s*(-?\d+)", re.MULTILINE)


def load_version(schema_dir=SCHEMA_DIR):
    """Return the API version string from version.txt, e.g. "14.0.0"."""
    text = (Path(schema_dir) / "version.txt").read_text(encoding="utf-8")
    match = re.search(r"JSONRPC_VERSION\s+(\S+)", text)
    if match is None:
        raise ValueError("version.txt does not contain JSONRPC_VERSION")
    return match.group(1)


def _join_literals(group):
    """Concatenate adjacent C++ string literals and undo escaping."""
    parts = re.findall(r'"((?:[^"\\]|\\.)*)"', group)
    joined = "".join(parts)
    return re.sub(r'\\(["\\])', r"\1", joined)


def _parse_status_enum(text):
    match = re.search(r"enum\s+JSONRPC_STATUS\s*\{(.*?)\}", text, re.DOTALL)
    if match is None:
        raise ValueError("JSONRPC_STATUS enum not found")
    body = re.sub(r"//[^\n]*", "", match.group(1))
    codes = {name: int(value) for name, value in _ENUM_ENTRY.findall(body)}
    if not codes:
        raise ValueError("JSONRPC_STATUS enum has no parsable entries")
    return codes


def load_error_taxonomy(header_path=UTILS_HEADER):
    """Parse JSONRPCUtils.h into a list of error descriptions.

    Each entry is {"name", "code", "message", "description", "has_data"},
    in the order of the JSONRPC_STATUS_DESCRIPTIONS array.
    """
    text = Path(header_path).read_text(encoding="utf-8")
    codes = _parse_status_enum(text)
    match = re.search(
        r"JSONRPC_STATUS_DESCRIPTIONS\s*\{\s*\{(.*?)\}\s*\}\s*;",
        text, re.DOTALL)
    if match is None:
        raise ValueError("JSONRPC_STATUS_DESCRIPTIONS array not found")
    taxonomy = []
    for entry in _DESCRIPTION_ENTRY.finditer(match.group(1)):
        enum_name, name, message, description, has_data = entry.groups()
        if enum_name not in codes:
            raise ValueError(f"unknown JSONRPC_STATUS value {enum_name}")
        taxonomy.append({
            "name": _join_literals(name),
            "code": codes[enum_name],
            "message": _join_literals(message),
            "description": _join_literals(description),
            "has_data": has_data == "true",
        })
    if not taxonomy:
        raise ValueError("JSONRPC_STATUS_DESCRIPTIONS has no parsable entries")
    return taxonomy


def _load_json(schema_dir, name):
    path = Path(schema_dir) / name
    with open(path, encoding="utf-8") as handle:
        return json.load(handle)


def collect_refs(schema, acc=None):
    """Return the set of type names referenced via "#/$defs/..." pointers."""
    if acc is None:
        acc = set()
    if isinstance(schema, dict):
        ref = schema.get("$ref")
        if isinstance(ref, str) and ref.startswith("#/$defs/"):
            acc.add(ref[len("#/$defs/"):])
        for value in schema.values():
            collect_refs(value, acc)
    elif isinstance(schema, list):
        for value in schema:
            collect_refs(value, acc)
    return acc


def load_service(schema_dir=SCHEMA_DIR):
    """Load the whole service description.

    Returns {"methods", "types", "notifications"}; the types map includes
    synthesized placeholders for the runtime-registered enum types.
    """
    methods = _load_json(schema_dir, "methods.json")
    types = _load_json(schema_dir, "types.json")
    notifications = _load_json(schema_dir, "notifications.json")
    unresolved = collect_refs([methods, types, notifications]) - set(types)
    unknown = unresolved - RUNTIME_ENUM_TYPES
    if unknown:
        raise ValueError("unresolved $ref: " + ", ".join(sorted(unknown)))
    for name in sorted(unresolved):
        types[name] = dict(RUNTIME_ENUM_PLACEHOLDER)
    return {
        "methods": methods,
        "types": types,
        "notifications": notifications,
    }


COMPONENT_SCHEMA_PREFIX = "#/components/schemas/"


def component_schemas(service, names=None):
    """The service's types as a components map, optionally only the named ones."""
    return {name: rewrite_refs(schema, COMPONENT_SCHEMA_PREFIX)
            for name, schema in service["types"].items()
            if names is None or name in names}


def rewrite_refs(schema, prefix):
    """Return a copy of schema with "#/$defs/X" refs rewritten to prefix+X."""
    if isinstance(schema, dict):
        result = {}
        for key, value in schema.items():
            if (key == "$ref" and isinstance(value, str)
                    and value.startswith("#/$defs/")):
                result[key] = prefix + value[len("#/$defs/"):]
            else:
                result[key] = rewrite_refs(value, prefix)
        return result
    if isinstance(schema, list):
        return [rewrite_refs(value, prefix) for value in schema]
    return schema


def namespace_of(method_name):
    """Return the namespace part of a method name, e.g. "Player"."""
    return method_name.split(".", 1)[0]


def returns_schema(returns):
    """Normalize a method's "returns" member to a schema object."""
    if isinstance(returns, str):
        if returns == "any":
            return {}
        return {"type": returns}
    return returns


def dump_document(document, path):
    """Serialize a generated document deterministically."""
    text = json.dumps(document, indent=2, ensure_ascii=False) + "\n"
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8", newline="\n")
