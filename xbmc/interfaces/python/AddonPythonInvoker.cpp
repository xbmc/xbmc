/*
 *  Copyright (C) 2013-2018 Team Kodi
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

#define RUNSCRIPT_SETUP_ENVIROMENT_VARIABLES \
  "" \
  "from os import environ\n" \
  "environ['SSL_CERT_FILE'] = 'system/certs/cacert.pem'\n" \
  ""

#define RUNSCRIPT_POSTSCRIPT \
        "print('-->Python Interpreter Initialized<--')\n" \
        ""

#if defined(TARGET_ANDROID)

#define RUNSCRIPT_COMPLIANT \
  RUNSCRIPT_PREAMBLE RUNSCRIPT_SETUPTOOLS_HACK RUNSCRIPT_POSTSCRIPT

#elif defined(TARGET_WINDOWS_STORE)

#define RUNSCRIPT_COMPLIANT \
  RUNSCRIPT_PREAMBLE RUNSCRIPT_SETUP_ENVIROMENT_VARIABLES RUNSCRIPT_POSTSCRIPT

#else

#define RUNSCRIPT_COMPLIANT \
  RUNSCRIPT_PREAMBLE RUNSCRIPT_POSTSCRIPT

#endif

namespace PythonBindings {
PyObject* PyInit_Module_xbmcdrm(void);
PyObject* PyInit_Module_xbmcgui(void);
PyObject* PyInit_Module_xbmc(void);
PyObject* PyInit_Module_xbmcplugin(void);
PyObject* PyInit_Module_xbmcaddon(void);
PyObject* PyInit_Module_xbmcvfs(void);
}

using namespace PythonBindings;

namespace
{
// clang-format off
const _inittab PythonModules[] =
  {
    { "xbmcdrm",    PyInit_Module_xbmcdrm    },
    { "xbmcgui",    PyInit_Module_xbmcgui    },
    { "xbmc",       PyInit_Module_xbmc       },
    { "xbmcplugin", PyInit_Module_xbmcplugin },
    { "xbmcaddon",  PyInit_Module_xbmcaddon  },
    { "xbmcvfs",    PyInit_Module_xbmcvfs    },
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
