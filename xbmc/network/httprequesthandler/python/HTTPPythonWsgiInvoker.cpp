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

#include "HTTPPythonWsgiInvoker.h"
#include "URL.h"
#include "addons/Webinterface.h"
#include "interfaces/legacy/wsgi/WsgiErrorStream.h"
#include "interfaces/legacy/wsgi/WsgiInputStream.h"
#include "interfaces/legacy/wsgi/WsgiResponse.h"
#include "utils/URIUtils.h"

#define MODULE      "xbmc"

#define RUNSCRIPT_PRAMBLE \
  "" \
  "import " MODULE "\n" \
  "xbmc.abortRequested = False\n" \
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
        MODULE ".log('-->HTTP Python WSGI Interpreter Initialized<--', " MODULE ".LOGNOTICE)\n" \
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
  void initModule_xbmcwsgi(void);
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
  { "xbmcwsgi",       initModule_xbmcwsgi }
};

#define PythonModulesSize sizeof(PythonModules) / sizeof(PythonModule)

CHTTPPythonWsgiInvoker::CHTTPPythonWsgiInvoker(ILanguageInvocationHandler* invocationHandler, HTTPPythonRequest* request)
  : CHTTPPythonInvoker(invocationHandler, request),
    m_wsgiResponse(NULL)
{ }

CHTTPPythonWsgiInvoker::~CHTTPPythonWsgiInvoker()
{
  delete m_wsgiResponse;
  m_wsgiResponse = NULL;
}

HTTPPythonRequest* CHTTPPythonWsgiInvoker::GetRequest()
{
  if (m_request == NULL || m_wsgiResponse == NULL)
    return NULL;

  if (m_internalError)
    return m_request;

  m_wsgiResponse->Finalize(m_request);
  return m_request;
}

void CHTTPPythonWsgiInvoker::executeScript(void *fp, const std::string &script, void *module, void *moduleDict)
{
  if (m_request == NULL || m_addon == NULL || m_addon->Type() != ADDON::ADDON_WEB_INTERFACE ||
      fp == NULL || script.empty() || module == NULL || moduleDict == NULL)
    return;

  ADDON::CWebinterface* webinterface = static_cast<ADDON::CWebinterface*>(m_addon.get());
  if (webinterface->GetType() != ADDON::WebinterfaceTypeWsgi)
  {
    CLog::Log(LOGERROR, "CHTTPPythonWsgiInvoker: trying to execute a non-WSGI script at %s", script.c_str());
    return;
  }

  PyObject* pyScript = NULL;
  PyObject* pyModule = NULL;
  PyObject* pyEntryPoint = NULL;
  std::map<std::string, std::string> cgiEnvironment;
  PyObject* pyEnviron = NULL;
  PyObject* pyStart_response = NULL;
  PyObject* pyArgs = NULL;
  PyObject* pyResult = NULL;
  PyObject* pyResultIterator = NULL;
  PyObject* pyIterResult = NULL;

  // get the script
  std::string scriptName = URIUtils::GetFileName(script);
  URIUtils::RemoveExtension(scriptName);
  pyScript = PyString_FromStringAndSize(scriptName.c_str(), scriptName.size());
  if (pyScript == NULL)
  {
    CLog::Log(LOGERROR, "CHTTPPythonWsgiInvoker: failed to convert script \"%s\" to python string", script.c_str());
    return;
  }

  // load the script
  CLog::Log(LOGDEBUG, "CHTTPPythonWsgiInvoker: loading WSGI script \"%s\"", script.c_str());
  pyModule = PyImport_Import(pyScript);
  Py_DECREF(pyScript);
  if (pyModule == NULL)
  {
    CLog::Log(LOGERROR, "CHTTPPythonWsgiInvoker: failed to load WSGI script \"%s\"", script.c_str());
    return;
  }

  // get the entry point
  const std::string& entryPoint = webinterface->EntryPoint();
  CLog::Log(LOGDEBUG, "CHTTPPythonWsgiInvoker: loading entry point \"%s\" from WSGI script \"%s\"", entryPoint.c_str(), script.c_str());
  pyEntryPoint = PyObject_GetAttrString(pyModule, entryPoint.c_str());
  if (pyEntryPoint == NULL)
  {
    CLog::Log(LOGERROR, "CHTTPPythonWsgiInvoker: failed to entry point \"%s\" from WSGI script \"%s\"", entryPoint.c_str(), script.c_str());
    goto cleanup;
  }

  // check if the loaded entry point is a callable function
  if (!PyCallable_Check(pyEntryPoint))
  {
    CLog::Log(LOGERROR, "CHTTPPythonWsgiInvoker: defined entry point \"%s\" from WSGI script \"%s\" is not callable", entryPoint.c_str(), script.c_str());
    goto cleanup;
  }

  // prepare the WsgiResponse object
  m_wsgiResponse = new XBMCAddon::xbmcwsgi::WsgiResponse();
  if (m_wsgiResponse == NULL)
  {
    CLog::Log(LOGERROR, "CHTTPPythonWsgiInvoker: failed to create WsgiResponse object for WSGI script \"%s\"", script.c_str());
    goto cleanup;
  }

  try
  {
    // prepare the start_response callable
    pyStart_response = PythonBindings::makePythonInstance(m_wsgiResponse, true);

    // create the (CGI) environment dictionary
    cgiEnvironment = createCgiEnvironment(m_request, m_addon);
    // and turn it into a python dictionary
    pyEnviron = PyDict_New();
    for (std::map<std::string, std::string>::const_iterator cgiEnv = cgiEnvironment.begin(); cgiEnv != cgiEnvironment.end(); ++cgiEnv)
    {
      PyObject* pyEnvEntry = PyString_FromStringAndSize(cgiEnv->second.c_str(), cgiEnv->second.size());
      PyDict_SetItemString(pyEnviron, cgiEnv->first.c_str(), pyEnvEntry);
      Py_DECREF(pyEnvEntry);
    }

    // add the WSGI-specific environment variables
    addWsgiEnvironment(m_request, pyEnviron);
  }
  catch (const XBMCAddon::WrongTypeException& e)
  {
    CLog::Log(LOGERROR, "CHTTPPythonWsgiInvoker: failed to prepare WsgiResponse object for WSGI script \"%s\" with wrong type exception: %s", script.c_str(), e.GetMessage());
    goto cleanup;
  }
  catch (const XbmcCommons::Exception& e)
  {
    CLog::Log(LOGERROR, "CHTTPPythonWsgiInvoker: failed to prepare WsgiResponse object for WSGI script \"%s\" with exception: %s", script.c_str(), e.GetMessage());
    goto cleanup;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CHTTPPythonWsgiInvoker: failed to prepare WsgiResponse object for WSGI script \"%s\" with unknown exception", script.c_str());
    goto cleanup;
  }

  // put together the arguments
  pyArgs = PyTuple_Pack(2, pyEnviron, pyStart_response);
  Py_DECREF(pyEnviron);
  Py_DECREF(pyStart_response);

  // call the given handler with the prepared arguments
  pyResult = PyObject_CallObject(pyEntryPoint, pyArgs);
  Py_DECREF(pyArgs);
  if (pyResult == NULL)
  {
    CLog::Log(LOGERROR, "CHTTPPythonWsgiInvoker: no result for WSGI script \"%s\"", script.c_str());
    goto cleanup;
  }

  // try to get an iterator from the result object
  pyResultIterator = PyObject_GetIter(pyResult);
  if (pyResultIterator == NULL || !PyIter_Check(pyResultIterator))
  {
    CLog::Log(LOGERROR, "CHTTPPythonWsgiInvoker: result of WSGI script \"%s\" is not iterable", script.c_str());
    goto cleanup;
  }

  // go through all the iterables in the result and turn them into strings
  while ((pyIterResult = PyIter_Next(pyResultIterator)) != NULL)
  {
    std::string result;
    try
    {
      PythonBindings::PyXBMCGetUnicodeString(result, pyIterResult, false, "result", "handle_request");
    }
    catch (const XBMCAddon::WrongTypeException& e)
    {
      CLog::Log(LOGERROR, "CHTTPPythonWsgiInvoker: failed to parse result iterable object for WSGI script \"%s\" with wrong type exception: %s", script.c_str(), e.GetMessage());
      goto cleanup;
    }
    catch (const XbmcCommons::Exception& e)
    {
      CLog::Log(LOGERROR, "CHTTPPythonWsgiInvoker: failed to parse result iterable object for WSGI script \"%s\" with exception: %s", script.c_str(), e.GetMessage());
      goto cleanup;
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "CHTTPPythonWsgiInvoker: failed to parse result iterable object for WSGI script \"%s\" with unknown exception", script.c_str());
      goto cleanup;
    }

    // append the result string to the response
    m_wsgiResponse->Append(result);
  }

cleanup:
  if (pyIterResult != NULL)
  {
    Py_DECREF(pyIterResult);
  }
  if (pyResultIterator != NULL)
  {
    // Call optional close method on iterator
    if (PyObject_HasAttrString(pyResultIterator, (char*)"close") == 1)
    {
      if (PyObject_CallMethod(pyResultIterator, (char*)"close", NULL) == NULL)
        CLog::Log(LOGERROR, "CHTTPPythonWsgiInvoker: failed to close iterator object for WSGI script \"%s\"", script.c_str());
    }
    Py_DECREF(pyResultIterator);
  }
  if (pyResult != NULL)
  {
    Py_DECREF(pyResult);
  }
  if (pyEntryPoint != NULL)
  {
    Py_DECREF(pyEntryPoint);
  }
  if (pyModule != NULL)
  {
    Py_DECREF(pyModule);
  }
}

std::map<std::string, CPythonInvoker::PythonModuleInitialization> CHTTPPythonWsgiInvoker::getModules() const
{
  static std::map<std::string, PythonModuleInitialization> modules;
  if (modules.empty())
  {
    for (size_t i = 0; i < PythonModulesSize; i++)
      modules.insert(std::make_pair(PythonModules[i].name, PythonModules[i].initialization));
  }

  return modules;
}

const char* CHTTPPythonWsgiInvoker::getInitializationScript() const
{
  return RUNSCRIPT;
}

std::map<std::string, std::string> CHTTPPythonWsgiInvoker::createCgiEnvironment(const HTTPPythonRequest* httpRequest, ADDON::AddonPtr addon)
{
  std::map<std::string, std::string> environment;

  // REQUEST_METHOD
  std::string requestMethod;
  switch (httpRequest->method)
  {
  case HEAD:
    requestMethod = "HEAD";
    break;

  case POST:
    requestMethod = "POST";
    break;

  case GET:
  default:
    requestMethod = "GET";
    break;
  }
  environment.insert(std::make_pair("REQUEST_METHOD", requestMethod));

  // SCRIPT_NAME
  std::string scriptName = std::dynamic_pointer_cast<ADDON::CWebinterface>(addon)->GetBaseLocation();
  environment.insert(std::make_pair("SCRIPT_NAME", scriptName));

  // PATH_INFO
  std::string pathInfo = httpRequest->path.substr(scriptName.size());
  environment.insert(std::make_pair("PATH_INFO", pathInfo));

  // QUERY_STRING
  CURL url(httpRequest->url);
  environment.insert(std::make_pair("QUERY_STRING", url.GetOptions()));

  // CONTENT_TYPE
  std::string headerValue;
  std::multimap<std::string, std::string>::const_iterator headerIt = httpRequest->headerValues.find(MHD_HTTP_HEADER_CONTENT_TYPE);
  if (headerIt != httpRequest->headerValues.end())
    headerValue = headerIt->second;
  environment.insert(std::make_pair("CONTENT_TYPE", headerValue));

  // CONTENT_LENGTH
  headerValue.clear();
  headerIt = httpRequest->headerValues.find(MHD_HTTP_HEADER_CONTENT_LENGTH);
  if (headerIt != httpRequest->headerValues.end())
    headerValue = headerIt->second;
  environment.insert(std::make_pair("CONTENT_LENGTH", headerValue));

  // SERVER_NAME
  environment.insert(std::make_pair("SERVER_NAME", httpRequest->hostname));

  // SERVER_PORT
  environment.insert(std::make_pair("SERVER_PORT", StringUtils::Format("%hu", httpRequest->port)));

  // SERVER_PROTOCOL
  environment.insert(std::make_pair("SERVER_PROTOCOL", httpRequest->version));

  // HTTP_<HEADER_NAME>
  for (headerIt = httpRequest->headerValues.begin(); headerIt != httpRequest->headerValues.end(); ++headerIt)
  {
    std::string headerName = headerIt->first;
    StringUtils::ToUpper(headerName);
    environment.insert(std::make_pair("HTTP_" + headerName, headerIt->second));
  }

  return environment;
}

void CHTTPPythonWsgiInvoker::addWsgiEnvironment(HTTPPythonRequest* request, void* environ)
{
  if (environ == NULL)
    return;

  PyObject* pyEnviron = reinterpret_cast<PyObject*>(environ);
  if (pyEnviron == NULL)
    return;

  // WSGI-defined variables
  {
    // wsgi.version
    PyObject* pyValue = Py_BuildValue("(ii)", 1, 0);
    PyDict_SetItemString(pyEnviron, "wsgi.version", pyValue);
    Py_DECREF(pyValue);
  }
  {
    // wsgi.url_scheme
    PyObject* pyValue = PyString_FromStringAndSize("http", 4);
    PyDict_SetItemString(pyEnviron, "wsgi.url_scheme", pyValue);
    Py_DECREF(pyValue);
  }
  {
    // wsgi.input
    XBMCAddon::xbmcwsgi::WsgiInputStream* wsgiInputStream = new XBMCAddon::xbmcwsgi::WsgiInputStream();
    if (request != NULL)
      wsgiInputStream->SetRequest(request);

    PythonBindings::prepareForReturn(wsgiInputStream);
    PyObject* pyWsgiInputStream = PythonBindings::makePythonInstance(wsgiInputStream, false);
    PyDict_SetItemString(pyEnviron, "wsgi.input", pyWsgiInputStream);
    Py_DECREF(pyWsgiInputStream);
  }
  {
    // wsgi.errors
    XBMCAddon::xbmcwsgi::WsgiErrorStream* wsgiErrorStream = new XBMCAddon::xbmcwsgi::WsgiErrorStream();
    if (request != NULL)
      wsgiErrorStream->SetRequest(request);

    PythonBindings::prepareForReturn(wsgiErrorStream);
    PyObject* pyWsgiErrorStream = PythonBindings::makePythonInstance(wsgiErrorStream, false);
    PyDict_SetItemString(pyEnviron, "wsgi.errors", pyWsgiErrorStream);
    Py_DECREF(pyWsgiErrorStream);
  }
  {
    // wsgi.multithread
    PyObject* pyValue = Py_BuildValue("b", false);
    PyDict_SetItemString(pyEnviron, "wsgi.multithread", pyValue);
    Py_DECREF(pyValue);
  }
  {
    // wsgi.multiprocess
    PyObject* pyValue = Py_BuildValue("b", false);
    PyDict_SetItemString(pyEnviron, "wsgi.multiprocess", pyValue);
    Py_DECREF(pyValue);
  }
  {
    // wsgi.run_once
    PyObject* pyValue = Py_BuildValue("b", true);
    PyDict_SetItemString(pyEnviron, "wsgi.run_once", pyValue);
    Py_DECREF(pyValue);
  }
}
