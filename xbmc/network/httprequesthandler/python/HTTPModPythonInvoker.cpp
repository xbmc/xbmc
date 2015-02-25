/*
 *      Copyright (C) 2015 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "interfaces/python/swig.h"

#include "HTTPModPythonInvoker.h"
#include "interfaces/legacy/mod_python/HttpRequest.h"

#define MODULE      "xbmc"
#define MODULE_HTTP "xbmcmod_python"

#define RUNSCRIPT_PRAMBLE \
        "" \
        "import " MODULE "\n" \
        "import " MODULE_HTTP "\n" \
        "class httpout:\n" \
        "  def write(self, data):\n" \
        "    req.write(data)\n" \
        "  def close(self):\n" \
        "    pass\n" \
        "  def flush(self):\n" \
        "    pass\n" \
        "class xbmcout:\n" \
        "  def __init__(self, loglevel=" MODULE ".LOGNOTICE):\n" \
        "    self.ll=loglevel\n" \
        "  def write(self, data):\n" \
        "    " MODULE ".log(data,self.ll)\n" \
        "  def close(self):\n" \
        "    " MODULE ".log('.')\n" \
        "  def flush(self):\n" \
        "    " MODULE ".log('.')\n" \
        "import sys\n" \
        "sys.stdout = httpout()\n" \
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
        MODULE ".log('-->HTTP ModPython Interpreter Initialized<--', " MODULE ".LOGNOTICE)\n" \
        ""

#if defined(TARGET_ANDROID)
#define RUNSCRIPT \
  RUNSCRIPT_PRAMBLE RUNSCRIPT_SETUPTOOLS_HACK RUNSCRIPT_POSTSCRIPT
#else
#define RUNSCRIPT \
  RUNSCRIPT_PRAMBLE RUNSCRIPT_POSTSCRIPT
#endif

namespace PythonBindings {
  void initModule_xbmc(void);
  void initModule_xbmcmod_python(void);
}

using namespace PythonBindings;

typedef struct
{
  const char *name;
  CPythonInvoker::PythonModuleInitialization initialization;
} PythonModule;

static PythonModule PythonModules[] =
{
  { "xbmc",           initModule_xbmc },
  { "xbmcmod_python", initModule_xbmcmod_python }
};

#define PythonModulesSize sizeof(PythonModules) / sizeof(PythonModule)

CHTTPModPythonInvoker::CHTTPModPythonInvoker(ILanguageInvocationHandler *invocationHandler, HTTPPythonRequest *request)
  : CHTTPPythonInvoker(invocationHandler, request),
    m_httpRequest(NULL)
{ }

CHTTPModPythonInvoker::~CHTTPModPythonInvoker()
{
  delete m_httpRequest;
  m_httpRequest = NULL;
}

HTTPPythonRequest* CHTTPModPythonInvoker::GetRequest()
{
  if (m_request == NULL || m_httpRequest == NULL)
    return NULL;

  if (m_internalError)
    return m_httpRequest->m_request;

  return m_httpRequest->Finalize();
}

std::map<std::string, CPythonInvoker::PythonModuleInitialization> CHTTPModPythonInvoker::getModules() const
{
  static std::map<std::string, PythonModuleInitialization> modules;
  if (modules.empty())
  {
    for (size_t i = 0; i < PythonModulesSize; i++)
      modules.insert(std::make_pair(PythonModules[i].name, PythonModules[i].initialization));
  }

  return modules;
}

const char* CHTTPModPythonInvoker::getInitializationScript() const
{
  return RUNSCRIPT;
}

void CHTTPModPythonInvoker::onPythonModuleInitialization(void* moduleDict)
{
  CHTTPPythonInvoker::onPythonModuleInitialization(moduleDict);

  if (m_request == NULL)
    return;

  m_httpRequest = new XBMCAddon::xbmcmod_python::HttpRequest();
  if (m_httpRequest == NULL)
  {
    CLog::Log(LOGERROR, "CHTTPModPythonInvoker: failed to create HttpRequest object for %s", m_request->url.c_str());
    return;
  }

  m_httpRequest->SetRequest(m_request);

  try
  {
    PythonBindings::prepareForReturn(m_httpRequest);

    PyObject* pyHttpRequest = PythonBindings::makePythonInstance(m_httpRequest, false);
    if (pyHttpRequest == NULL)
    {
      CLog::Log(LOGERROR, "CHTTPModPythonInvoker: failed to create Python HttpRequest object for %s", m_request->url.c_str());
      return;
    }

    // set the global HttpRequest object for this request
    PyDict_SetItem((PyObject *)moduleDict, PyString_FromString("req"), pyHttpRequest);
  }
  catch (const XBMCAddon::WrongTypeException& e)
  {
    CLog::Log(LOGERROR, "CHTTPModPythonInvoker: failed with wrong type exception: %s", e.GetMessage());
  }
  catch (const XbmcCommons::Exception& e)
  {
    CLog::Log(LOGERROR, "CHTTPModPythonInvoker: failed with exception: %s", e.GetMessage());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CHTTPModPythonInvoker: failed with unknown exception");
  }
}
