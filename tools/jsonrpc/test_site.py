#!/usr/bin/env python3
#
#  Copyright (C) 2026 Team Kodi
#  This file is part of Kodi - https://kodi.tv
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#  See LICENSES/README.md for more information.
#
"""Smoke tests for the static documentation site generator.

Generates the site into a temporary directory and checks the page
inventory, that every internal link resolves to an emitted file, that the
machine-readable artifacts are byte-identical copies of the docs/jsonrpc
originals, and that no href/src attribute uses an absolute path.
"""

import json
import posixpath
import tempfile
import unittest
from html.parser import HTMLParser
from pathlib import Path

import generate_site
import kodi_schema
import markup


class LinkCollector(HTMLParser):

    def __init__(self):
        super().__init__()
        self.links = []

    def handle_starttag(self, tag, attrs):
        for key, value in attrs:
            if key in ("href", "src") and value is not None:
                self.links.append(value)


def is_external(link):
    return link.startswith(("http://", "https://", "mailto:"))


class TestSiteGeneration(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.tempdir = tempfile.TemporaryDirectory()
        cls.out = Path(cls.tempdir.name) / "site"
        cls.written = generate_site.generate(cls.out)
        cls.vdir = "v" + kodi_schema.load_version().split(".")[0]
        cls.service = kodi_schema.load_service()

    @classmethod
    def tearDownClass(cls):
        cls.tempdir.cleanup()

    def html_names(self, folder):
        directory = self.out / self.vdir / folder
        return {path.name for path in directory.glob("*.html")}

    def test_method_page_count(self):
        names = self.html_names("methods")
        self.assertIn("index.html", names)
        self.assertEqual(len(names - {"index.html"}), len(self.service["methods"]))

    def test_notification_page_count(self):
        names = self.html_names("notifications")
        self.assertIn("index.html", names)
        self.assertEqual(
            len(names - {"index.html"}), len(self.service["notifications"]))

    def test_type_page_count(self):
        names = self.html_names("types")
        self.assertIn("index.html", names)
        self.assertEqual(len(names - {"index.html"}), len(self.service["types"]))

    def test_core_files_exist(self):
        for relative in ("index.html", "style.css", ".nojekyll",
                         f"{self.vdir}/errors.html",
                         f"{self.vdir}/openrpc.json",
                         f"{self.vdir}/asyncapi.json"):
            with self.subTest(relative=relative):
                self.assertTrue((self.out / relative).is_file())

    def test_artifact_copies_are_byte_identical(self):
        for artifact in ("openrpc.json", "asyncapi.json"):
            with self.subTest(artifact=artifact):
                original = (generate_site.DOCS_DIR / artifact).read_bytes()
                copy = (self.out / self.vdir / artifact).read_bytes()
                self.assertEqual(original, copy)

    def test_internal_links_resolve_and_are_relative(self):
        pages = sorted(self.out.rglob("*.html"))
        self.assertTrue(pages)
        for page in pages:
            relative = page.relative_to(self.out).as_posix()
            collector = LinkCollector()
            collector.feed(page.read_text(encoding="utf-8"))
            for link in collector.links:
                if is_external(link):
                    continue
                with self.subTest(page=relative, link=link):
                    self.assertFalse(link.startswith("/"),
                                     "absolute path in href/src")
                    self.assertNotIn(":", link.split("/")[0].split("#")[0],
                                     "unexpected scheme or drive letter")
                    target = link.split("#", 1)[0]
                    if not target:
                        continue
                    resolved = posixpath.normpath(
                        posixpath.join(posixpath.dirname(relative), target))
                    self.assertFalse(resolved.startswith(".."),
                                     "link escapes the site root")
                    self.assertTrue((self.out / resolved).is_file(),
                                    f"broken link to {resolved}")

    def test_runtime_enum_pages_mention_introspect(self):
        service = kodi_schema.load_service()
        placeholders = [name for name, schema in service["types"].items()
                        if schema.get("x-kodi-runtime-enum")]
        self.assertGreater(len(placeholders), 0)
        for name in placeholders:
            with self.subTest(type=name):
                text = (self.out / self.vdir / "types"
                        / f"{name}.html").read_text(encoding="utf-8")
                self.assertIn("JSONRPC.Introspect", text)
                self.assertIn("runtime", text)

    def test_prose_documents_render_as_pages(self):
        for source, title in generate_site.PROSE_DOCUMENTS.items():
            page = self.out / self.vdir / (source[:-len(".md")] + ".html")
            with self.subTest(document=source):
                self.assertTrue(page.is_file())
                text = page.read_text(encoding="utf-8")
                self.assertIn(markup.esc(title), text)
                # nothing left unrendered
                self.assertNotIn("**", text)
                self.assertNotIn("```", text)

    def test_markdown_renders_the_forms_the_documents_use(self):
        rendered = markup.md_to_html(
            "# Title\n"
            "\n"
            "A paragraph with `code`, **bold** and [a link](x.html).\n"
            "\n"
            "- first item\n"
            "- second item\n"
            "\n"
            "| a | b |\n"
            "|---|---|\n"
            "| 1 | 2 |\n"
            "\n"
            "```json\n"
            "{\"k\": 1}\n"
            "```\n"
            "\n"
            "---\n")
        self.assertIn("<h1>Title</h1>", rendered)
        self.assertIn("<code>code</code>", rendered)
        self.assertIn("<strong>bold</strong>", rendered)
        self.assertIn('<a href="x.html">a link</a>', rendered)
        self.assertIn("<li>first item</li>", rendered)
        self.assertIn("<th>a</th>", rendered)
        self.assertIn("<td>1</td>", rendered)
        self.assertIn("<hr>", rendered)
        self.assertIn("&quot;k&quot;", rendered)
        # the alignment row of a table is not data
        self.assertNotIn("<td>---</td>", rendered)

    def test_markdown_emphasis_can_wrap_a_code_span(self):
        rendered = markup.md_to_html("**`Thing.Method`** does a thing\n")
        self.assertIn("<strong><code>Thing.Method</code></strong>", rendered)
        self.assertNotIn("**", rendered)

    def test_markdown_leaves_markup_inside_code_alone(self):
        rendered = markup.md_to_html("Literal `**not bold**` here\n")
        self.assertIn("<code>**not bold**</code>", rendered)
        self.assertNotIn("<strong>", rendered)

    def test_landing_page_names_every_runtime_enum(self):
        text = (self.out / "index.html").read_text(encoding="utf-8")
        service = kodi_schema.load_service()
        placeholders = [name for name, schema in service["types"].items()
                        if schema.get("x-kodi-runtime-enum")]
        self.assertGreater(len(placeholders), 0)
        self.assertIn(f"{len(placeholders)} types carry no values", text)
        for name in placeholders:
            with self.subTest(type=name):
                self.assertIn(f"<code>{markup.esc(name)}</code>", text)

    def test_landing_page_explains_introspect_against_the_artifacts(self):
        text = (self.out / "index.html").read_text(encoding="utf-8")
        self.assertIn("Discovering the API at runtime", text)
        # the three things only the live call can answer
        for claim in ("runtime enumeration", "What your connection may call",
                      "Which version you are talking to"):
            with self.subTest(claim=claim):
                self.assertIn(claim, text)

    def test_landing_page_leads_with_the_socket_transports(self):
        text = (self.out / "index.html").read_text(encoding="utf-8")
        order = [text.index(f"<strong>{name}</strong>")
                 for name in ("WebSocket", "Raw TCP", "HTTP")]
        self.assertEqual(order, sorted(order))
        self.assertIn("Which to use", text)

    def test_landing_page_does_not_claim_the_socket_is_authenticated(self):
        """Authentication is described as covering HTTP only."""
        text = (self.out / "index.html").read_text(encoding="utf-8")
        self.assertIn("Authentication over HTTP is basic auth", text)
        self.assertNotIn("Authentication is HTTP basic auth", text)

    def test_landing_page_renders_every_example(self):
        text = (self.out / "index.html").read_text(encoding="utf-8")
        examples = sorted(generate_site.EXAMPLES_DIR.glob("*.json"))
        self.assertTrue(examples)
        curl_count = text.count("curl -X POST http://localhost:8080/jsonrpc")
        method_examples = 0
        for path in examples:
            with open(path, encoding="utf-8") as handle:
                example = json.load(handle)
            with self.subTest(example=path.name):
                self.assertIn(markup.esc(example["title"]), text)
            if "method" in example:
                method_examples += 1
        self.assertEqual(curl_count, method_examples)

    def test_errors_page_lists_whole_taxonomy(self):
        text = (self.out / self.vdir
                / "errors.html").read_text(encoding="utf-8")
        for error in kodi_schema.load_error_taxonomy():
            with self.subTest(error=error["name"]):
                self.assertIn(f"<code>{error['code']}</code>", text)
                self.assertIn(markup.esc(error["name"]), text)

    def test_method_pages_list_their_declared_errors(self):
        for name, method in self.service["methods"].items():
            page = (self.out / self.vdir / "methods"
                    / f"{name}.html").read_text(encoding="utf-8")
            with self.subTest(method=name):
                for error in method["errors"]:
                    self.assertIn(f'href="../errors.html#{error}"', page)
                if not method["errors"]:
                    self.assertNotIn('href="../errors.html#', page)

    def test_output_is_deterministic(self):
        with tempfile.TemporaryDirectory() as second:
            again = Path(second) / "site"
            generate_site.generate(again)
            first_files = sorted(path.relative_to(self.out).as_posix()
                                 for path in self.out.rglob("*")
                                 if path.is_file())
            second_files = sorted(path.relative_to(again).as_posix()
                                  for path in again.rglob("*")
                                  if path.is_file())
            self.assertEqual(first_files, second_files)
            for relative in first_files:
                with self.subTest(file=relative):
                    self.assertEqual((self.out / relative).read_bytes(),
                                     (again / relative).read_bytes())


class TestMarkup(unittest.TestCase):

    def test_inline_forms(self):
        rendered = markup.md_inline("a *b* **c** `*d*` [e](f) 2*3*4")
        self.assertEqual(rendered, 'a <em>b</em> <strong>c</strong> <code>*d*</code> '
                                   '<a href="f">e</a> 2*3*4')

if __name__ == "__main__":
    unittest.main()
