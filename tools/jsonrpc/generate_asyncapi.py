#!/usr/bin/env python3
#
#  Copyright (C) 2026 Team Kodi
#  This file is part of Kodi - https://kodi.tv
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#  See LICENSES/README.md for more information.
#
"""Generate docs/jsonrpc/asyncapi.json from the JSON-RPC service description.

Produces an AsyncAPI 3.0.0 document covering the server-initiated
notifications, which Kodi pushes over WebSocket and raw TCP only.
Request/response methods are documented separately in openrpc.json by
generate_openrpc.py.
"""

import kodi_schema

OUTPUT_PATH = kodi_schema.REPO_ROOT / "docs" / "jsonrpc" / "asyncapi.json"

SCHEMA_PREFIX = kodi_schema.COMPONENT_SCHEMA_PREFIX

DESCRIPTION = (
    "Server-initiated notifications of Kodi's JSON-RPC 2.0 API. Kodi pushes "
    "these over the WebSocket and raw TCP transports only; they are never "
    "delivered over HTTP. Request/response methods are documented separately "
    "in openrpc.json.")


def build_message(name, notification):
    properties = {}
    required = []
    for descriptor in notification.get("params", []):
        properties[descriptor["name"]] = kodi_schema.rewrite_refs(
            descriptor["schema"], SCHEMA_PREFIX)
        if descriptor.get("required"):
            required.append(descriptor["name"])
    return {
        "name": name,
        "summary": notification["description"],
        "payload": {
            "type": "object",
            "properties": {
                "jsonrpc": {"const": "2.0"},
                "method": {"const": name},
                "params": {
                    "type": "object",
                    "properties": properties,
                    "required": required,
                    "additionalProperties": False,
                },
            },
            "required": ["jsonrpc", "method", "params"],
        },
    }


def schema_closure(notifications, types):
    """Return the type names transitively referenced by the notifications."""
    pending = sorted(kodi_schema.collect_refs(notifications))
    closure = set()
    while pending:
        name = pending.pop()
        if name in closure:
            continue
        closure.add(name)
        for referenced in sorted(kodi_schema.collect_refs(types[name])):
            if referenced not in closure:
                pending.append(referenced)
    return closure


def build_document():
    version = kodi_schema.load_version()
    service = kodi_schema.load_service()
    notifications = service["notifications"]
    messages = {name: build_message(name, notification)
                for name, notification in notifications.items()}
    closure = schema_closure(notifications, service["types"])
    schemas = kodi_schema.component_schemas(service, closure)
    channel_messages = {name: {"$ref": "#/components/messages/" + name}
                        for name in notifications}
    operations = {}
    for name in notifications:
        operations[name] = {
            "action": "send",
            "channel": {"$ref": "#/channels/jsonrpc"},
            "messages": [{"$ref": "#/channels/jsonrpc/messages/" + name}],
        }
    return {
        "asyncapi": "3.0.0",
        "info": {
            "title": "Kodi JSON-RPC notifications",
            "version": version,
            "description": DESCRIPTION,
            "license": {"name": "GPL-2.0-or-later"},
        },
        "servers": {
            "websocket": {
                "host": "{host}:9090",
                "pathname": "/jsonrpc",
                "protocol": "ws",
                "variables": {"host": {"default": "localhost"}},
            },
            "tcp": {
                "host": "{host}:9090",
                "protocol": "tcp",
                "description": ("Raw TCP transport; newline-free JSON-RPC "
                                "objects over a plain socket."),
                "variables": {"host": {"default": "localhost"}},
            },
        },
        "channels": {
            "jsonrpc": {
                "description": ("The single JSON-RPC channel; every "
                                "notification is delivered on it."),
                "messages": channel_messages,
            },
        },
        "operations": operations,
        "components": {
            "messages": messages,
            "schemas": schemas,
        },
    }


def main():
    kodi_schema.dump_document(build_document(), OUTPUT_PATH)


if __name__ == "__main__":
    main()
