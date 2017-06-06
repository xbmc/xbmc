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
        "xbmc.abortRequested = False\n" \
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

namespace PythonBindings {
PyObject* PyInit_Module_xbmcdrm(void);
PyObject* PyInit_Module_xbmcgui(void);
PyObject* PyInit_Module_xbmc(void);
PyObject* PyInit_Module_xbmcplugin(void);
PyObject* PyInit_Module_xbmcaddon(void);
PyObject* PyInit_Module_xbmcvfs(void);
}

using namespace PythonBindings;

typedef struct
{
  const char *name;
  CPythonInvoker::PythonModuleInitialization initialization;
} PythonModule;

static PythonModule PythonModules[] =
  {
    { "xbmcdrm",    PyInit_Module_xbmcdrm    },
    { "xbmcgui",    PyInit_Module_xbmcgui    },
    { "xbmc",       PyInit_Module_xbmc       },
    { "xbmcplugin", PyInit_Module_xbmcplugin },
    { "xbmcaddon",  PyInit_Module_xbmcaddon  },
    { "xbmcvfs",    PyInit_Module_xbmcvfs    }
  };

CAddonPythonInvoker::CAddonPythonInvoker(ILanguageInvocationHandler *invocationHandler)
  : CPythonInvoker(invocationHandler)
{
  PyImport_AppendInittab("xbmcdrm", PyInit_Module_xbmcdrm);
  PyImport_AppendInittab("xbmcgui", PyInit_Module_xbmcgui);
  PyImport_AppendInittab("xbmc", PyInit_Module_xbmc);
  PyImport_AppendInittab("xbmcplugin", PyInit_Module_xbmcplugin);
  PyImport_AppendInittab("xbmcaddon", PyInit_Module_xbmcaddon);
  PyImport_AppendInittab("xbmcvfs", PyInit_Module_xbmcvfs);
}

CAddonPythonInvoker::~CAddonPythonInvoker() = default;

std::map<std::string, CPythonInvoker::PythonModuleInitialization> CAddonPythonInvoker::getModules() const
{
  static std::map<std::string, PythonModuleInitialization> modules;
  if (modules.empty())
  {
    for (const PythonModule& pythonModule : PythonModules)
      modules.insert(std::make_pair(pythonModule.name, pythonModule.initialization));
  }

  return modules;
}

const char* CAddonPythonInvoker::getInitializationScript() const
{
  return RUNSCRIPT_COMPLIANT;
}
