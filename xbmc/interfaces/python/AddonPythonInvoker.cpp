/*
 *  Copyright (C) 2013-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// python.h should always be included first before any other includes
#include "AddonPythonInvoker.h"

#include <utility>

#include <Python.h>
#include <osdefs.h>

#define MODULE "xbmc"

#define RUNSCRIPT_PREAMBLE \
        "" \
        "import " MODULE "\n" \
        "class xbmcout:\n" \
        "  def __init__(self, loglevel=" MODULE ".LOGDEBUG):\n" \
        "    self.ll=loglevel\n" \
        "  def write(self, data):\n" \
        "    " MODULE ".log(data,self.ll)\n" \
        "  def close(self):\n" \
        "    " MODULE ".log('.')\n" \
        "  def flush(self):\n" \
        "    " MODULE ".log('.')\n" \
        "import sys\n" \
        "sys.stdout = xbmcout()\n" \
        "sys.stderr = xbmcout(" MODULE ".LOGERROR)\n" \
        ""

#define RUNSCRIPT_SETUPTOOLS_HACK \
  "" \
  "import types,sys\n" \
  "pkg_resources_code = \\\n" \
  "\"\"\"\n" \
  "def resource_filename(__name__,__path__):\n" \
  "  return __path__\n" \
  "\"\"\"\n" \
  "pkg_resources = types.ModuleType('pkg_resources')\n" \
  "exec(pkg_resources_code, pkg_resources.__dict__)\n" \
  "sys.modules['pkg_resources'] = pkg_resources\n" \
  ""

#define RUNSCRIPT_POSTSCRIPT \
        "print('-->Python Interpreter Initialized<--')\n" \
        ""

#if defined(TARGET_ANDROID)

#define RUNSCRIPT_COMPLIANT \
  RUNSCRIPT_PREAMBLE RUNSCRIPT_SETUPTOOLS_HACK RUNSCRIPT_POSTSCRIPT

#else

#define RUNSCRIPT_COMPLIANT \
  RUNSCRIPT_PREAMBLE RUNSCRIPT_POSTSCRIPT

#endif

extern "C"
{
  PyObject* PyInit_xbmcdrm(void);
  PyObject* PyInit_xbmcgui(void);
  PyObject* PyInit_xbmc(void);
  PyObject* PyInit_xbmcplugin(void);
  PyObject* PyInit_xbmcaddon(void);
  PyObject* PyInit_xbmcvfs(void);
}

namespace
{
// clang-format off
const _inittab PythonModules[] =
  {
    { "xbmcdrm",    PyInit_xbmcdrm    },
    { "xbmcgui",    PyInit_xbmcgui    },
    { "xbmc",       PyInit_xbmc       },
    { "xbmcplugin", PyInit_xbmcplugin },
    { "xbmcaddon",  PyInit_xbmcaddon  },
    { "xbmcvfs",    PyInit_xbmcvfs    },
    { nullptr,      nullptr }
  };
// clang-format on
} // namespace

CAddonPythonInvoker::CAddonPythonInvoker(ILanguageInvocationHandler *invocationHandler)
  : CPythonInvoker(invocationHandler)
{
}

CAddonPythonInvoker::~CAddonPythonInvoker() = default;

void CAddonPythonInvoker::GlobalInitializeModules(void)
{
  if (PyImport_ExtendInittab(const_cast<_inittab*>(PythonModules)))
    CLog::Log(LOGWARNING, "CAddonPythonInvoker(): unable to extend inittab");
}

const char* CAddonPythonInvoker::getInitializationScript() const
{
  return RUNSCRIPT_COMPLIANT;
}
