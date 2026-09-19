#!/usr/bin/env python3
#
#  Copyright (C) 2026 Team Kodi
#  This file is part of Kodi - https://kodi.tv
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#  See LICENSES/README.md for more information.
#
"""Tests for the machine-readable API description generators.

The error taxonomy parser is exercised against the real JSONRPCUtils.h.
"""

import json
import re
import shutil
import tempfile
import unittest
from pathlib import Path

import generate_asyncapi
import generate_openrpc
import kodi_schema

EXAMPLES_DIR = Path(__file__).resolve().parent / "examples"


def serialize(document):
    return json.dumps(document, indent=2, ensure_ascii=False) + "\n"


def walk_refs(node, acc):
    if isinstance(node, dict):
        ref = node.get("$ref")
        if isinstance(ref, str):
            acc.append(ref)
        for value in node.values():
            walk_refs(value, acc)
    elif isinstance(node, list):
        for value in node:
            walk_refs(value, acc)
    return acc


class TestErrorTaxonomyParser(unittest.TestCase):

    def setUp(self):
        self.taxonomy = kodi_schema.load_error_taxonomy()

    def test_entry_count(self):
        self.assertEqual(len(self.taxonomy), 10)

    def test_expected_codes(self):
        by_name = {entry["name"]: entry for entry in self.taxonomy}
        self.assertEqual(by_name["InvalidParams"]["code"], -32602)
        self.assertTrue(by_name["InvalidParams"]["has_data"])
        self.assertEqual(by_name["FailedToExecute"]["code"], -32100)
        self.assertEqual(by_name["AccessDenied"]["code"], -32096)

    def test_all_fields_populated(self):
        for entry in self.taxonomy:
            self.assertTrue(entry["name"])
            self.assertTrue(entry["message"])
            self.assertTrue(entry["description"])
            self.assertIsInstance(entry["code"], int)
            self.assertIsInstance(entry["has_data"], bool)

    def test_split_literals_are_joined(self):
        by_name = {entry["name"]: entry for entry in self.taxonomy}
        description = by_name["MethodNotFound"]["description"]
        self.assertIn("not available over the transport", description)
        self.assertNotIn('"  "', description)

    def test_escaped_quotes_are_unescaped(self):
        by_name = {entry["name"]: entry for entry in self.taxonomy}
        self.assertIn('"data"', by_name["InvalidParams"]["description"])


class TestOpenRpcDocument(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.document = generate_openrpc.build_document()

    def test_deterministic(self):
        again = generate_openrpc.build_document()
        self.assertEqual(serialize(self.document), serialize(again))

    def test_output_file_is_current(self):
        on_disk = generate_openrpc.OUTPUT_PATH.read_text(encoding="utf-8")
        self.assertEqual(on_disk, serialize(self.document))

    def test_method_count(self):
        expected = len(kodi_schema.load_service()["methods"])
        self.assertEqual(len(self.document["methods"]), expected)

    def test_all_refs_resolve(self):
        schemas = self.document["components"]["schemas"]
        errors = self.document["components"]["errors"]
        refs = walk_refs(self.document, [])
        self.assertTrue(refs)
        for ref in refs:
            if ref.startswith("#/components/schemas/"):
                name = ref[len("#/components/schemas/"):]
                self.assertIn(name, schemas)
            elif ref.startswith("#/components/errors/"):
                name = ref[len("#/components/errors/"):]
                self.assertIn(name, errors)
            else:
                self.fail(f"unexpected ref form: {ref}")

    def test_runtime_enums_are_placeholders(self):
        schemas = self.document["components"]["schemas"]
        self.assertTrue(schemas["Input.Action"].get("x-kodi-runtime-enum"))
        self.assertTrue(schemas["GUI.Window"].get("x-kodi-runtime-enum"))


class TestUnresolvedReferences(unittest.TestCase):

    def test_runtime_enums_are_the_ones_the_code_registers(self):
        source = (kodi_schema.REPO_ROOT / "xbmc" / "interfaces" / "json-rpc"
                  / "JSONRPC.cpp").read_text(encoding="utf-8")
        registered = set(re.findall(r'AddEnum\("([^"]+)"', source))
        self.assertEqual(registered, set(kodi_schema.RUNTIME_ENUM_TYPES))

    def test_a_reference_to_nothing_is_an_error(self):
        with tempfile.TemporaryDirectory() as tempdir:
            schema_dir = Path(tempdir)
            for name in ("methods.json", "types.json", "notifications.json"):
                shutil.copy(kodi_schema.SCHEMA_DIR / name, schema_dir / name)
            methods = json.loads((schema_dir / "methods.json").read_text(encoding="utf-8"))
            methods["JSONRPC.Version"]["returns"] = {"$ref": "#/$defs/Nonexistent.Type"}
            (schema_dir / "methods.json").write_text(json.dumps(methods), encoding="utf-8")
            with self.assertRaises(ValueError) as raised:
                kodi_schema.load_service(schema_dir)
            self.assertIn("Nonexistent.Type", str(raised.exception))


class TestAsyncApiDocument(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.document = generate_asyncapi.build_document()

    def test_deterministic(self):
        again = generate_asyncapi.build_document()
        self.assertEqual(serialize(self.document), serialize(again))

    def test_output_file_is_current(self):
        on_disk = generate_asyncapi.OUTPUT_PATH.read_text(encoding="utf-8")
        self.assertEqual(on_disk, serialize(self.document))

    def test_counts(self):
        expected = len(kodi_schema.load_service()["notifications"])
        self.assertEqual(len(self.document["operations"]), expected)
        self.assertEqual(len(self.document["components"]["messages"]), expected)
        self.assertEqual(
            len(self.document["channels"]["jsonrpc"]["messages"]), expected)

    def test_all_refs_resolve(self):
        schemas = self.document["components"]["schemas"]
        messages = self.document["components"]["messages"]
        channel_messages = self.document["channels"]["jsonrpc"]["messages"]
        refs = walk_refs(self.document, [])
        self.assertTrue(refs)
        for ref in refs:
            if ref.startswith("#/components/schemas/"):
                name = ref[len("#/components/schemas/"):]
                self.assertIn(name, schemas)
            elif ref.startswith("#/components/messages/"):
                name = ref[len("#/components/messages/"):]
                self.assertIn(name, messages)
            elif ref.startswith("#/channels/jsonrpc/messages/"):
                name = ref[len("#/channels/jsonrpc/messages/"):]
                self.assertIn(name, channel_messages)
            elif ref == "#/channels/jsonrpc":
                self.assertIn("jsonrpc", self.document["channels"])
            else:
                self.fail(f"unexpected ref form: {ref}")

    def test_schema_closure_is_closed(self):
        schemas = self.document["components"]["schemas"]
        refs = walk_refs(schemas, [])
        for ref in refs:
            self.assertTrue(ref.startswith("#/components/schemas/"))
            self.assertIn(ref[len("#/components/schemas/"):], schemas)


class TestExamples(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.service = kodi_schema.load_service()
        cls.examples = []
        for path in sorted(EXAMPLES_DIR.glob("*.json")):
            with open(path, encoding="utf-8") as handle:
                cls.examples.append((path.name, json.load(handle)))

    def test_examples_exist(self):
        self.assertTrue(self.examples)

    def test_method_examples(self):
        methods = self.service["methods"]
        for filename, example in self.examples:
            if "method" not in example:
                continue
            with self.subTest(filename=filename):
                name = example["method"]
                self.assertIn(name, methods)
                self.assertEqual(example["request"]["method"], name)
                self.assertEqual(example["request"]["jsonrpc"], "2.0")
                self.assertEqual(example["response"]["jsonrpc"], "2.0")
                self.assertEqual(example["request"]["id"],
                                 example["response"]["id"])
                declared = {descriptor["name"] for descriptor
                            in methods[name].get("params", [])}
                given = set(example["request"].get("params", {}))
                self.assertLessEqual(given, declared)

    def test_notification_examples(self):
        notifications = self.service["notifications"]
        for filename, example in self.examples:
            if "notification" not in example:
                continue
            with self.subTest(filename=filename):
                name = example["notification"]
                self.assertIn(name, notifications)
                message = example["message"]
                self.assertEqual(message["jsonrpc"], "2.0")
                self.assertEqual(message["method"], name)
                declared = {descriptor["name"] for descriptor
                            in notifications[name].get("params", [])}
                self.assertLessEqual(set(message["params"]), declared)


if __name__ == "__main__":
    unittest.main()
