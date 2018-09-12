/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// python.h should always be included first before any other includes
#include <Python.h>
#include <osdefs.h>

#include "AddonPythonInvoker.h"

#include <utility>

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
  "import imp,sys\n" \
  "pkg_resources_code = \\\n" \
  "\"\"\"\n" \
  "def resource_filename(__name__,__path__):\n" \
  "  return __path__\n" \
  "\"\"\"\n" \
  "pkg_resources = imp.new_module('pkg_resources')\n" \
  "exec pkg_resources_code in pkg_resources.__dict__\n" \
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
  void initModule_xbmcdrm(void);
  void initModule_xbmcgui(void);
  void initModule_xbmc(void);
  void initModule_xbmcplugin(void);
  void initModule_xbmcaddon(void);
  void initModule_xbmcvfs(void);
}

using namespace PythonBindings;

typedef struct
{
  const char *name;
  CPythonInvoker::PythonModuleInitialization initialization;
} PythonModule;

static PythonModule PythonModules[] =
  {
    { "xbmcdrm",    initModule_xbmcdrm    },
    { "xbmcgui",    initModule_xbmcgui    },
    { "xbmc",       initModule_xbmc       },
    { "xbmcplugin", initModule_xbmcplugin },
    { "xbmcaddon",  initModule_xbmcaddon  },
    { "xbmcvfs",    initModule_xbmcvfs    }
  };

CAddonPythonInvoker::CAddonPythonInvoker(ILanguageInvocationHandler *invocationHandler)
  : CPythonInvoker(invocationHandler)
{ }

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
