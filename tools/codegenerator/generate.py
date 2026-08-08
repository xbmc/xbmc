"""Driver: SWIG xml -> transform -> seed type table -> emit -> write .cpp.

Replaces `groovy.ui.GroovyMain Generator.groovy <xml> <template> <cpp>`.
Usage: generate.py <module>.i.xml <module>.i.cpp
"""

from __future__ import annotations

import sys
import xml.etree.ElementTree as ET

import swig_type_parser as stp
import emit
from transform import transform_swig_xml


def main(argv):
    if len(argv) != 3:
        sys.stderr.write("usage: generate.py <module>.i.xml <module>.i.cpp\n")
        return 2
    xmlpath, outpath = argv[1], argv[2]
    root = ET.parse(xmlpath).getroot()
    module = transform_swig_xml(root)

    stp.reset_type_table()
    for tt in module.kids("typetab"):
        stp.append_type_table(tt.kids("entry"))

    out = emit.generate(module)
    # newline="" so \n is never translated to \r\n on Windows hosts: Groovy's
    # File.write emits \n on every platform, and the goldens are \n. utf-8 keeps
    # the byte stream host-locale-independent (generated bindings are ASCII).
    with open(outpath, "w", encoding="utf-8", newline="") as f:
        f.write(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
