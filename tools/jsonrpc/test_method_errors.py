#!/usr/bin/env python3
#
#  Copyright (C) 2026 Team Kodi
#  This file is part of Kodi - https://kodi.tv
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#  See LICENSES/README.md for more information.
#
"""Tests for the per-method error derivation.

The declarations in methods.json are checked against the real handler
sources; when they drift, run method_errors.py --write and commit the result.
"""

import tempfile
import textwrap
import unittest
from pathlib import Path

import kodi_schema
import method_errors


class TestDeclarations(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.methods = kodi_schema.load_service()["methods"]
        cls.taxonomy = {error["name"]
                        for error in kodi_schema.load_error_taxonomy()}
        cls.derived = method_errors.derive()

    def test_every_method_declares_a_list(self):
        for name, method in self.methods.items():
            with self.subTest(method=name):
                self.assertIsInstance(method.get("errors"), list)

    def test_declared_names_are_in_the_taxonomy(self):
        for name, method in self.methods.items():
            with self.subTest(method=name):
                self.assertTrue(set(method["errors"]) <= self.taxonomy)

    def test_pre_dispatch_names_are_in_the_taxonomy(self):
        self.assertTrue(set(method_errors.PRE_DISPATCH) <= self.taxonomy)

    def test_derivation_covers_every_method(self):
        self.assertEqual(set(self.derived), set(self.methods))

    def test_declarations_match_the_handlers(self):
        for name, errors in self.derived.items():
            with self.subTest(method=name):
                self.assertEqual(self.methods[name]["errors"], errors)


class TestDerivation(unittest.TestCase):

    SOURCE = textwrap.dedent("""
        // return NotFound in a comment does not count
        JSONRPC_STATUS Helper(const CVariant& value)
        {
          // return AccessDenied in a comment does not count
          const char* text = "return Unavailable";
          return value.isNull() ? NotFound : OK;
        }

        JSONRPC_STATUS CBase::Inherited(const CVariant& value)
        {
          return value.isNull() ? BadPermission : OK;
        }

        JSONRPC_STATUS CTest::Inner(const CVariant& value)
        {
          if (!Helper(value))
            return InternalError;
          return OK;
        }

        JSONRPC_STATUS CTest::Outer(const std::string& method, ITransportLayer* transport,
                                    IClient* client, const CVariant& parameterObject,
                                    CVariant& result)
        {
          CDatabase database;
          if (!database.Open())
            return FailedToExecute;
          const JSONRPC_STATUS inherited = Inherited(parameterObject);
          if (inherited != OK)
            return inherited;
          return Inner(parameterObject);
        }

        JSONRPC_STATUS CTest::Open(const std::string& method, ITransportLayer* transport,
                                   IClient* client, const CVariant& parameterObject,
                                   CVariant& result)
        {
          return Unavailable;
        }

        JSONRPC_STATUS CTest::Declared(const CVariant& value);
        """)

    METHOD_MAP = textwrap.dedent("""
        JsonRpcMethodMap CJSONServiceDescription::m_methodMaps[] = {
          { "Test.Outer", CTest::Outer },
          { "Test.Open",  CTest::Open },
        };
        """)

    def setUp(self):
        self.tempdir = tempfile.TemporaryDirectory()
        source_dir = Path(self.tempdir.name)
        (source_dir / "Test.cpp").write_text(self.SOURCE, encoding="utf-8")
        (source_dir / method_errors.METHOD_MAP.name).write_text(
            self.METHOD_MAP, encoding="utf-8")
        self.derived = method_errors.derive(source_dir)

    def tearDown(self):
        self.tempdir.cleanup()

    def test_statuses_travel_through_called_functions(self):
        self.assertEqual(self.derived["Test.Outer"],
                         ["InternalError", "FailedToExecute", "BadPermission", "NotFound"])

    def test_a_bare_call_reaches_an_inherited_static(self):
        self.assertIn("BadPermission", self.derived["Test.Outer"])

    def test_a_member_call_is_not_a_handler_call(self):
        # database.Open() must not pull in CTest::Open's Unavailable
        self.assertNotIn("Unavailable", self.derived["Test.Outer"])

    def test_comments_and_strings_are_ignored(self):
        self.assertNotIn("AccessDenied", self.derived["Test.Outer"])
        self.assertNotIn("Unavailable", self.derived["Test.Outer"])
        self.assertEqual(self.derived["Test.Open"], ["Unavailable"])


if __name__ == "__main__":
    unittest.main()
