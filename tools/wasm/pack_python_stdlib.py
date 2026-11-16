#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Packs the Python standard library for the WASM virtual filesystem.

usage: pack_python_stdlib.py <stdlib directory> <zip file>

Modules are stored compiled: Python cannot cache bytecode in a zip, so it
would compile the sources on every import.
"""

import os
import py_compile
import sys
import tempfile
import zipfile

# Installer and desktop tools, which cannot be used at run time, and
# site-packages, which is not on the module search path inside a zip.
EXCLUDED = {
    "__pycache__",
    "ensurepip",
    "idlelib",
    "pydoc_data",
    "site-packages",
    "tkinter",
    "turtle.py",
    "turtledemo",
    "venv",
}


def compiled(path, name):
    with tempfile.TemporaryDirectory() as tmp:
        cfile = py_compile.compile(
            path,
            cfile=os.path.join(tmp, "module.pyc"),
            dfile=name,
            doraise=True,
            invalidation_mode=py_compile.PycInvalidationMode.UNCHECKED_HASH,
        )
        with open(cfile, "rb") as file:
            return file.read()


def fail(error):
    raise error


def main():
    stdlib, output = sys.argv[1:]
    version = "python%d.%d" % sys.version_info[:2]
    if os.path.basename(os.path.normpath(stdlib)) != version:
        sys.exit("%s has to be compiled by its own Python version, not %s" % (stdlib, version))

    with zipfile.ZipFile(output, "w") as archive:
        for root, dirs, files in os.walk(stdlib, onerror=fail):
            dirs[:] = sorted(d for d in dirs if d not in EXCLUDED)
            for entry in sorted(f for f in files if f not in EXCLUDED):
                path = os.path.join(root, entry)
                name = os.path.relpath(path, stdlib).replace(os.sep, "/")
                if name.endswith(".py"):
                    name, data = name + "c", compiled(path, name)
                else:
                    with open(path, "rb") as file:
                        data = file.read()
                archive.writestr(zipfile.ZipInfo(name), data, zipfile.ZIP_DEFLATED)


if __name__ == "__main__":
    main()
