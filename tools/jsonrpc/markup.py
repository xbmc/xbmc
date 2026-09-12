#!/usr/bin/env python3
#
#  Copyright (C) 2026 Team Kodi
#  This file is part of Kodi - https://kodi.tv
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#  See LICENSES/README.md for more information.
#
"""HTML escaping, JSON blocks and the Markdown subset the prose documents
use.
"""

import html
import json
import re


def esc(text):
    return html.escape(str(text), quote=True)


def dumps(value):
    return json.dumps(value, ensure_ascii=False)


def pre_json(value):
    text = json.dumps(value, indent=2, ensure_ascii=False)
    return f"<pre><code>{esc(text)}</code></pre>"


_MD_INLINE_CODE = re.compile(r"`([^`]+)`")
_MD_BOLD = re.compile(r"\*\*([^*]+)\*\*")
_MD_ITALIC = re.compile(r"(?<![*\w])\*([^*\s](?:[^*]*[^*\s])?)\*(?![*\w])")
_MD_LINK = re.compile(r"\[([^\]]+)\]\(([^)]+)\)")


def md_inline(text):
    """Inline markdown: code spans, bold, italics, links.

    Code spans are lifted out first so their content is never read as
    markup.
    """
    spans = []

    def lift(match):
        spans.append(match.group(1))
        return f"\x00{len(spans) - 1}\x00"

    lifted = _MD_INLINE_CODE.sub(lift, text)

    rendered = esc(lifted)
    rendered = _MD_BOLD.sub(r"<strong>\1</strong>", rendered)
    rendered = _MD_ITALIC.sub(r"<em>\1</em>", rendered)
    rendered = _MD_LINK.sub(r'<a href="\2">\1</a>', rendered)

    for index, span in enumerate(spans):
        rendered = rendered.replace(f"\x00{index}\x00",
                                    f"<code>{esc(span)}</code>")
    return rendered


def md_to_html(source, link_rewrite=None):
    """Render the markdown subset the JSON-RPC documents use.

    Headings, paragraphs, bullet lists, fenced code, tables, horizontal
    rules and the inline forms above. Anything else is emitted as a
    paragraph.
    """
    if link_rewrite:
        for before, after in link_rewrite.items():
            source = source.replace(f"]({before})", f"]({after})")

    parts = []
    lines = source.splitlines()
    index = 0
    while index < len(lines):
        line = lines[index]

        if line.startswith("```"):
            index += 1
            block = []
            while index < len(lines) and not lines[index].startswith("```"):
                block.append(lines[index])
                index += 1
            index += 1
            parts.append(f"<pre><code>{esc(chr(10).join(block))}</code></pre>")
            continue

        if line.startswith("#"):
            level = len(line) - len(line.lstrip("#"))
            parts.append(f"<h{level}>{md_inline(line[level:].strip())}"
                         f"</h{level}>")
            index += 1
            continue

        if line.strip() in ("---", "***", "___"):
            parts.append("<hr>")
            index += 1
            continue

        if line.startswith("|"):
            rows = []
            while index < len(lines) and lines[index].startswith("|"):
                rows.append(lines[index])
                index += 1
            parts.append(_md_table(rows))
            continue

        if line.lstrip().startswith("- "):
            items = []
            while index < len(lines) and (lines[index].lstrip().startswith("- ")
                                          or (lines[index].startswith("  ")
                                              and lines[index].strip()
                                              and items)):
                current = lines[index]
                if current.lstrip().startswith("- "):
                    items.append(current.lstrip()[2:])
                else:
                    # a wrapped continuation of the item above
                    items[-1] += " " + current.strip()
                index += 1
            body = "".join(f"<li>{md_inline(item)}</li>" for item in items)
            parts.append(f"<ul>{body}</ul>")
            continue

        if not line.strip():
            index += 1
            continue

        paragraph = []
        while index < len(lines) and lines[index].strip() \
                and not lines[index].startswith(("#", "|", "```")) \
                and not lines[index].lstrip().startswith("- "):
            paragraph.append(lines[index].strip())
            index += 1
        parts.append(f"<p>{md_inline(' '.join(paragraph))}</p>")

    return "".join(parts)


def _md_table(rows):
    cells = [[cell.strip() for cell in row.strip().strip("|").split("|")]
             for row in rows]
    # the second row of a markdown table is the alignment rule, not data
    body = cells[2:] if len(cells) > 1 and set("-: |") >= set(rows[1]) else \
        cells[1:]
    head = "".join(f"<th>{md_inline(cell)}</th>" for cell in cells[0])
    out = [f"<thead><tr>{head}</tr></thead><tbody>"]
    for row in body:
        out.append("<tr>"
                   + "".join(f"<td>{md_inline(cell)}</td>" for cell in row)
                   + "</tr>")
    out.append("</tbody>")
    return f'<div class="tablewrap"><table>{"".join(out)}</table></div>'
