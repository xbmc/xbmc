#!/usr/bin/env python3
#
#  Copyright (C) 2026 Team Kodi
#  This file is part of Kodi - https://kodi.tv
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#  See LICENSES/README.md for more information.
#
"""Check that the JSON-RPC service description uses no draft-03 construct.

The C++ parser reads JSON Schema 2020-12; a draft-03 keyword is silently
ignored or degraded, so this is what CI runs to keep one out.

Usage: python tools/jsonrpc/lint_schema.py [--schema-dir DIR]
"""

import argparse
import json
import sys
from pathlib import Path

SCHEMA_FILES = ("methods.json", "types.json", "notifications.json")
REF_PREFIX = "#/$defs/"
SIMPLE_TYPES = frozenset(
    ("array", "boolean", "integer", "null", "number", "object", "string")
)


def lint_schema(data, path, problems):
    if not isinstance(data, dict):
        return
    if "extends" in data:
        problems.append(f"{path}: draft-03 'extends'")
    if "id" in data:
        problems.append(f"{path}: draft-03 inline 'id'")
    if "enums" in data:
        problems.append(f"{path}: non-standard 'enums'")
    if "divisibleBy" in data:
        problems.append(f"{path}: draft-03 'divisibleBy'")
    if "additionalItems" in data:
        problems.append(f"{path}: draft-03 'additionalItems'")
    if isinstance(data.get("items"), list):
        problems.append(f"{path}: draft-03 tuple-form 'items'")
    for bound in ("exclusiveMinimum", "exclusiveMaximum"):
        if isinstance(data.get(bound), bool):
            problems.append(f"{path}: draft-03 boolean '{bound}'")
    if "required" in data and not isinstance(data["required"], list):
        # 2020-12 spells requiredness as an array on the containing object; a
        # draft-03 flag on the property itself is ignored by the C++ parser.
        problems.append(f"{path}: 'required' is {data['required']!r}, not an array of "
                        "property names - move it to the containing object")
    ref = data.get("$ref")
    if isinstance(ref, str) and not ref.startswith(REF_PREFIX):
        problems.append(f"{path}: unqualified $ref {ref!r}")
    type_value = data.get("type")
    if isinstance(type_value, list) and any(isinstance(m, dict) for m in type_value):
        problems.append(f"{path}: draft-03 union type array")
    if isinstance(type_value, str) and type_value not in SIMPLE_TYPES:
        # Anything else is draft-03 ("any", or a type name in place of a $ref)
        # or a misspelling; the C++ parser degrades it to AnyValue.
        problems.append(f"{path}: 'type' is {type_value!r}, not a JSON Schema type - "
                        "omit it for an unconstrained schema, $ref a defined type, "
                        "or fix the spelling")
    for key in ("items", "additionalProperties"):
        if isinstance(data.get(key), dict):
            lint_schema(data[key], f"{path}/{key}", problems)
    if isinstance(data.get("properties"), dict):
        for name, prop in data["properties"].items():
            lint_schema(prop, f"{path}/properties/{name}", problems)
    for group in ("anyOf", "allOf"):
        if isinstance(data.get(group), list):
            for i, member in enumerate(data[group]):
                lint_schema(member, f"{path}/{group}[{i}]", problems)


def lint_param(param, path, problems):
    if not isinstance(param, dict):
        problems.append(f"{path}: param is not an object")
        return
    unexpected = set(param) - {"name", "required", "description", "schema"}
    if unexpected:
        problems.append(f"{path}: flattened param (unexpected keys "
                        f"{sorted(unexpected)}); expected a content descriptor")
        return
    if "schema" not in param:
        problems.append(f"{path}: param has no schema")
        return
    if "required" in param and param["required"] is not True:
        problems.append(f"{path}: param 'required' must be true or absent")
    lint_schema(param["schema"], f"{path}/schema", problems)


def lint(schemas):
    """Return the problems in {filename: parsed JSON} for the three schema files."""
    problems = []

    # types must stay topologically ordered: the C++ parser resolves an
    # out-of-order allOf reference against an unparsed stub
    seen = set()
    for name, definition in schemas["types.json"].items():
        for member in definition.get("allOf", []) or []:
            ref = member.get("$ref", "")
            base = ref[len(REF_PREFIX):] if ref.startswith(REF_PREFIX) else ref
            if base not in seen:
                problems.append(f"types.json:{name}: allOf references {base} "
                                "before it is defined")
        seen.add(name)

    for filename in SCHEMA_FILES:
        for name, definition in schemas[filename].items():
            if definition.get("type") in ("method", "notification"):
                for i, param in enumerate(definition.get("params", []) or []):
                    lint_param(param, f"{filename}:{name}/params[{i}]", problems)
                returns = definition.get("returns")
                if isinstance(returns, dict):
                    lint_schema(returns, f"{filename}:{name}/returns", problems)
            else:
                lint_schema(definition, f"{filename}:{name}", problems)
    return problems


def load(schema_dir):
    return {filename: json.loads((schema_dir / filename).read_text(encoding="utf-8"))
            for filename in SCHEMA_FILES}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--schema-dir", type=Path,
                        default=Path(__file__).resolve().parents[2]
                        / "xbmc" / "interfaces" / "json-rpc" / "schema")
    args = parser.parse_args()

    problems = lint(load(args.schema_dir))
    for problem in problems:
        print(f"lint: {problem}", file=sys.stderr)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
