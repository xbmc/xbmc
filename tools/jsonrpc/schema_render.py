#!/usr/bin/env python3
#
#  Copyright (C) 2026 Team Kodi
#  This file is part of Kodi - https://kodi.tv
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#  See LICENSES/README.md for more information.
#
"""Renders a schema fragment as an inline phrase, a properties table or an
envelope placeholder.
"""

from markup import dumps, esc, pre_json

DEFS_PREFIX = "#/$defs/"

CONSTRAINT_KEYS = ("minimum", "maximum", "exclusiveMinimum",
                   "exclusiveMaximum", "minLength", "maxLength", "minItems",
                   "maxItems", "multipleOf", "format")


def ref_name(schema):
    return schema["$ref"][len(DEFS_PREFIX):]


def constraints_span(schema):
    parts = []
    for key in CONSTRAINT_KEYS:
        if key in schema:
            parts.append(f"{key}: {dumps(schema[key])}")
    if schema.get("uniqueItems"):
        parts.append("unique items")
    if not parts:
        return ""
    return f' <span class="constraints">({esc("; ".join(parts))})</span>'


def is_simple(schema):
    """True when the schema renders to a single inline phrase."""
    if not isinstance(schema, dict):
        return True
    if "$ref" in schema:
        return True
    if "anyOf" in schema:
        return all(is_simple(branch) for branch in schema["anyOf"])
    if "allOf" in schema or "properties" in schema:
        return False
    if isinstance(schema.get("additionalProperties"), dict):
        return False
    if schema.get("type") == "array":
        return is_simple(schema.get("items", {}))
    return True



def members_table(rows):
    """A name/type/required/default/description table around the given rows."""
    return (
        '<div class="tablewrap"><table>'
        "<thead><tr><th>Name</th><th>Type</th><th>Required</th>"
        "<th>Default</th><th>Description</th></tr></thead>"
        f"<tbody>{''.join(rows)}</tbody>"
        "</table></div>")

class SchemaRenderer:
    """Renders schema fragments; `vdir` is the major-version directory type
    links point into."""

    def __init__(self, vdir):
        self.vdir = vdir

    def type_link(self, name, depth):
        href = "../" * depth + f"{self.vdir}/types/{name}.html"
        return f'<a href="{esc(href)}">{esc(name)}</a>'

    def render_inline(self, schema, depth):
        """Render a schema as an inline phrase (type-column style)."""
        if not isinstance(schema, dict) or not schema:
            return "any"
        if "$ref" in schema:
            return self.type_link(ref_name(schema), depth) \
                + constraints_span(schema)
        if "anyOf" in schema:
            branches = schema["anyOf"]
            if all(is_simple(branch) for branch in branches):
                return " | ".join(self.render_inline(branch, depth)
                                  for branch in branches)
            items = "".join(f"<li>{self.render_inline(branch, depth)}</li>"
                            for branch in branches)
            return f'<ul class="branches">{items}</ul>'
        if "allOf" in schema:
            bases = ", ".join(self.render_inline(base, depth)
                              for base in schema["allOf"])
            own = ", ".join(esc(name) for name in schema.get("properties", ()))
            suffix = f' <span class="props">{{{own}}}</span>' if own else ""
            return f"object extending {bases}{suffix}"
        if "enum" in schema:
            values = " | ".join(f"<code>{esc(dumps(value))}</code>"
                                for value in schema["enum"])
            return values + constraints_span(schema)
        stype = schema.get("type")
        if isinstance(stype, list):
            return esc(" | ".join(stype)) + constraints_span(schema)
        if stype == "array":
            items = self.render_inline(schema.get("items", {}), depth)
            return f"array of {items}" + constraints_span(schema)
        if stype == "object":
            names = ", ".join(esc(name)
                              for name in schema.get("properties", ()))
            if names:
                return f'object <span class="props">{{{names}}}</span>'
            extra = schema.get("additionalProperties")
            if isinstance(extra, dict):
                rendered = self.render_inline(extra, depth)
                return f"object of {rendered} values"
            return "object"
        if stype is None:
            return "any"
        return esc(stype) + constraints_span(schema)

    def properties_table(self, schema, depth):
        required = set(schema.get("required", ()))
        rows = []
        for name, prop in schema.get("properties", {}).items():
            prop = prop if isinstance(prop, dict) else {}
            default = ""
            if "default" in prop:
                default = f"<code>{esc(dumps(prop['default']))}</code>"
            description = esc(prop.get("description", ""))
            if prop.get("deprecated"):
                description = ('<span class="badge warn">deprecated</span> '
                               + description)
            rows.append(
                "<tr>"
                f"<td><code>{esc(name)}</code></td>"
                f"<td>{self.render_inline(prop, depth)}</td>"
                f"<td>{'Yes' if name in required else 'No'}</td>"
                f"<td>{default}</td>"
                f"<td>{description}</td>"
                "</tr>")
        extra = schema.get("additionalProperties")
        if isinstance(extra, dict):
            rows.append(
                "<tr>"
                "<td><em>any name</em></td>"
                f"<td>{self.render_inline(extra, depth)}</td>"
                "<td>No</td><td></td>"
                f"<td>{esc(extra.get('description', ''))}</td>"
                "</tr>")
        return (
            '<div class="tablewrap"><table>'
            "<thead><tr><th>Name</th><th>Type</th><th>Required</th>"
            "<th>Default</th><th>Description</th></tr></thead>"
            f"<tbody>{''.join(rows)}</tbody>"
            "</table></div>")

    def render_block(self, schema, depth):
        """Render a schema as a block (returns section / type pages)."""
        if not isinstance(schema, dict) or not schema:
            return "<p>any</p>"
        parts = []
        if schema.get("description"):
            parts.append(f'<p class="muted">{esc(schema["description"])}</p>')
        if "allOf" in schema:
            bases = ", ".join(self.render_inline(base, depth)
                              for base in schema["allOf"])
            parts.append(f"<p>Extends {bases}</p>")
        if "properties" in schema \
                or isinstance(schema.get("additionalProperties"), dict):
            parts.append(self.properties_table(schema, depth))
        elif "enum" in schema:
            items = "".join(f"<li><code>{esc(dumps(value))}</code></li>"
                            for value in schema["enum"])
            parts.append(f'<ul class="enum-list">{items}</ul>'
                         + constraints_span(schema))
        elif "anyOf" in schema and not is_simple(schema):
            items = "".join(f"<li>{self.render_block(branch, depth)}</li>"
                            for branch in schema["anyOf"])
            parts.append(f'<ol class="branches">{items}</ol>')
        elif schema.get("type") == "array" \
                and not is_simple(schema.get("items", {})):
            parts.append("<p>array of" + constraints_span(schema) + ":</p>")
            parts.append(self.render_block(schema.get("items", {}), depth))
        elif "allOf" not in schema:
            parts.append(f"<p>{self.render_inline(schema, depth)}</p>")
        return "".join(parts)

    def placeholder(self, schema):
        """Plain-text type placeholder for an envelope skeleton."""
        if not isinstance(schema, dict) or not schema:
            return "any"
        if "$ref" in schema:
            return ref_name(schema)
        if "anyOf" in schema:
            labels = list(dict.fromkeys(self.placeholder(branch)
                                        for branch in schema["anyOf"]))
            return " | ".join(labels)
        if "enum" in schema:
            values = [dumps(value) for value in schema["enum"]]
            if len(values) > 4:
                values = values[:4] + ["..."]
            return " | ".join(values)
        stype = schema.get("type")
        if isinstance(stype, list):
            return " | ".join(stype)
        if stype == "array":
            return f"array of {self.placeholder(schema.get('items', {}))}"
        if stype == "object" and schema.get("properties"):
            return "{" + ", ".join(schema["properties"]) + "}"
        return stype or "any"

    def envelope_skeleton(self, name, entity, notification=False):
        envelope = {"jsonrpc": "2.0"}
        if not notification:
            envelope["id"] = 1
        envelope["method"] = name
        params = entity.get("params", [])
        if params:
            envelope["params"] = {
                descriptor["name"]:
                    f"<{self.placeholder(descriptor['schema'])}>"
                for descriptor in params}
        return envelope

    def params_table(self, params, depth):
        rows = []
        for descriptor in params:
            schema = descriptor.get("schema", {})
            default = ""
            source = descriptor if "default" in descriptor else schema
            if isinstance(source, dict) and "default" in source:
                default = f"<code>{esc(dumps(source['default']))}</code>"
            description = descriptor.get("description") \
                or (schema.get("description", "")
                    if isinstance(schema, dict) else "")
            rows.append(
                "<tr>"
                f"<td><code>{esc(descriptor['name'])}</code></td>"
                f"<td>{self.render_inline(schema, depth)}</td>"
                f"<td>{'Yes' if descriptor.get('required') else 'No'}</td>"
                f"<td>{default}</td>"
                f"<td>{esc(description)}</td>"
                "</tr>")
        return members_table(rows)

    def raw_schema_details(self, entity):
        return (f"<details><summary>Schema</summary>{pre_json(entity)}"
                "</details>")
