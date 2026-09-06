# Python bindings

The seven Python modules addons import (xbmc, xbmcgui, xbmcplugin, xbmcaddon,
xbmcvfs, xbmcdrm, xbmcwsgi) are generated here by stock SWIG from the
AddonModule*.i files, wrapping the C++ API in xbmc/interfaces/legacy/. The
kodi_*.i files carry the typemaps and policies shared by all modules; start
with kodi_common.i, which includes the rest in dependency order and documents
each one.

## Requirements

SWIG 4.5.0 or newer (CMakeLists.txt enforces the floor). fix-swig-3535.cmake
additionally pins the exact versions its output rewrite has been verified
against, and fails the build on any other, so bumping SWIG means re-verifying
that script.

## Sub-interpreter safety

Kodi runs every addon in its own CPython sub-interpreter, concurrently. Stock
SWIG's -builtin runtime is not safe under that (upstream swig/swig#3535); two
measures here make it safe:

- SwigRuntime.cpp + the SWIG_LINK_RUNTIME define: modules find the shared
  type table through a plain C function returning a self-linked static ring
  head, instead of importing a Python capsule that lives and dies with one
  interpreter. This uses an existing ifdef in SWIG's emitted runtime; do not
  confuse it with SWIG's Lib/linkruntime.c, whose stub is broken and whose
  registration contract this design deliberately avoids.
- fix-swig-3535.cmake, run on each generated file after SWIG: rewrites the
  per-class init blocks so each builtin Python type is created once per
  process and shared immortally by every interpreter, matching the semantics
  of the retired Groovy-generated bindings.

XBPython::Initialize imports all seven modules in the main interpreter at
startup so first registration happens in an interpreter that never exits.

