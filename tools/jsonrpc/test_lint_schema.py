#!/usr/bin/env python3
#
#  Copyright (C) 2026 Team Kodi
#  This file is part of Kodi - https://kodi.tv
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#  See LICENSES/README.md for more information.
#
"""Tests for the draft-03 lint over the JSON-RPC service description."""

import copy
import unittest

import kodi_schema
import lint_schema


class TestLintSchema(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.schemas = lint_schema.load(kodi_schema.SCHEMA_DIR)

    def test_the_shipped_schema_is_clean(self):
        self.assertEqual(lint_schema.lint(self.schemas), [])

    def lint_with(self, filename, name, **changes):
        schemas = copy.deepcopy(self.schemas)
        schemas[filename][name].update(changes)
        return lint_schema.lint(schemas)

    def test_extends_is_reported(self):
        problems = self.lint_with("types.json", "Library.Id", extends="Item.Fields.Base")
        self.assertTrue(any("draft-03 'extends'" in p for p in problems), problems)

    def test_a_boolean_required_is_reported(self):
        problems = self.lint_with("types.json", "Library.Id", required=True)
        self.assertTrue(any("'required' is True" in p for p in problems), problems)

    def test_an_unqualified_reference_is_reported(self):
        problems = self.lint_with("methods.json", "JSONRPC.Version",
                                  returns={"$ref": "Library.Id"})
        self.assertTrue(any("unqualified $ref" in p for p in problems), problems)

    def test_tuple_items_and_boolean_bounds_are_reported(self):
        problems = self.lint_with("types.json", "Library.Id",
                                  items=[{"type": "string"}], exclusiveMinimum=True)
        self.assertTrue(any("tuple-form 'items'" in p for p in problems), problems)
        self.assertTrue(any("boolean 'exclusiveMinimum'" in p for p in problems), problems)

    def test_a_type_defined_after_the_type_extending_it_is_reported(self):
        schemas = copy.deepcopy(self.schemas)
        types = schemas["types.json"]
        types["Zzz.Base"] = {"type": "object", "properties": {"a": {"type": "string"}}}
        types["Aaa.Derived"] = {"allOf": [{"$ref": "#/$defs/Zzz.Base"}]}
        reordered = {"Aaa.Derived": types.pop("Aaa.Derived")}
        reordered.update(types)
        schemas["types.json"] = reordered
        problems = lint_schema.lint(schemas)
        self.assertTrue(any("before it is defined" in p for p in problems), problems)


if __name__ == "__main__":
    unittest.main()
