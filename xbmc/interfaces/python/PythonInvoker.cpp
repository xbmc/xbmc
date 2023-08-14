/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// clang-format off
// python.h should always be included first before any other includes
#include <mutex>
#include <Python.h>
// clang-format on

#include "PythonInvoker.h"

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/python/PyContext.h"
#include "interfaces/python/pythreadstate.h"
#include "interfaces/python/swig.h"
#include "messaging/ApplicationMessenger.h"
#include "threads/SingleLock.h"
#include "threads/SystemClock.h"
#include "utils/CharsetConverter.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

// clang-format off
// This breaks fmt because of SEP define, don't include
// before anything that includes logging
#include <osdefs.h>
// clang-format on

#include <cassert>
#include <iterator>

#ifdef TARGET_WINDOWS
extern "C" FILE* fopen_utf8(const char* _Filename, const char* _Mode);
#else
#define fopen_utf8 fopen
#endif

#define GC_SCRIPT \
  "import gc\n" \
  "gc.collect(2)\n"

#define PY_PATH_SEP DELIM

// Time before ill-behaved scripts are terminated
#define PYTHON_SCRIPT_TIMEOUT 5000ms // ms

using namespace XFILE;
using namespace std::chrono_literals;

static const std::string getListOfAddonClassesAsString(
    XBMCAddon::AddonClass::Ref<XBMCAddon::Python::PythonLanguageHook>& languageHook)
{
  std::string message;
  std::unique_lock<CCriticalSection> l(*(languageHook.get()));
  const std::set<XBMCAddon::AddonClass*>& acs = languageHook->GetRegisteredAddonClasses();
  bool firstTime = true;
  for (const auto& iter : acs)
  {
    if (!firstTime)
      message += ",";
    else
      firstTime = false;
    message += iter->GetClassname();
  }

  return message;
}

CPythonInvoker::CPythonInvoker(ILanguageInvocationHandler* invocationHandler)
  : ILanguageInvoker(invocationHandler), m_threadState(NULL)
{
}

CPythonInvoker::~CPythonInvoker()
{
  // nothing to do for the default invoker used for registration with the
  // CScriptInvocationManager
  if (GetId() < 0)
    return;

  if (GetState() < InvokerStateExecutionDone)
    CLog::Log(LOGDEBUG, "CPythonInvoker({}): waiting for python thread \"{}\" to stop", GetId(),
              (!m_sourceFile.empty() ? m_sourceFile : "unknown script"));
  Stop(true);
  pulseGlobalEvent();

  onExecutionFinalized();
}

bool CPythonInvoker::Execute(
    const std::string& script,
    const std::vector<std::string>& arguments /* = std::vector<std::string>() */)
{
  if (script.empty())
    return false;

  if (!CFileUtils::Exists(script))
  {
    CLog::Log(LOGERROR, "CPythonInvoker({}): python script \"{}\" does not exist", GetId(),
              CSpecialProtocol::TranslatePath(script));
    return false;
  }

  if (!onExecutionInitialized())
    return false;

  return ILanguageInvoker::Execute(script, arguments);
}

bool CPythonInvoker::execute(const std::string& script, const std::vector<std::string>& arguments)
{
  std::vector<std::wstring> w_arguments;
  for (const auto& argument : arguments)
  {
    std::wstring w_argument;
    g_charsetConverter.utf8ToW(argument, w_argument, false);
    w_arguments.push_back(w_argument);
  }
  return execute(script, w_arguments);
}

bool CPythonInvoker::execute(const std::string& script, std::vector<std::wstring>& arguments)
{
  // copy the code/script into a local string buffer
  m_sourceFile = script;
  std::set<std::string> pythonPath;

  CLog::Log(LOGDEBUG, "CPythonInvoker({}, {}): start processing", GetId(), m_sourceFile);

  std::string realFilename(CSpecialProtocol::TranslatePath(m_sourceFile));
  std::string scriptDir = URIUtils::GetDirectory(realFilename);
  URIUtils::RemoveSlashAtEnd(scriptDir);

  // set m_threadState if it's not set.
  PyThreadState* l_threadState = nullptr;
  bool newInterp = false;
  {
    if (!m_threadState)
    {
#if PY_VERSION_HEX < 0x03070000
      // this is a TOTAL hack. We need the GIL but we need to borrow a PyThreadState in order to get it
      // as of Python 3.2 since PyEval_AcquireLock is deprecated
      extern PyThreadState* savestate;
      PyEval_RestoreThread(savestate);
#else
      PyThreadState* ts = PyInterpreterState_ThreadHead(PyInterpreterState_Main());
      PyEval_RestoreThread(ts);
#endif
      l_threadState = Py_NewInterpreter();
      PyEval_ReleaseThread(l_threadState);
      if (l_threadState == NULL)
      {
        CLog::Log(LOGERROR, "CPythonInvoker({}, {}): FAILED to get thread m_threadState!", GetId(),
                  m_sourceFile);
        return false;
      }
      newInterp = true;
    }
    else
      l_threadState = m_threadState;
  }

  // get the GIL
  PyEval_RestoreThread(l_threadState);
  if (newInterp)
  {
    m_languageHook = new XBMCAddon::Python::PythonLanguageHook(l_threadState->interp);
    m_languageHook->RegisterMe();

    onInitialization();
    setState(InvokerStateInitialized);

    if (realFilename == m_sourceFile)
      CLog::Log(LOGDEBUG, "CPythonInvoker({}, {}): the source file to load is \"{}\"", GetId(),
                m_sourceFile, m_sourceFile);
    else
      CLog::Log(LOGDEBUG, "CPythonInvoker({}, {}): the source file to load is \"{}\" (\"{}\")",
                GetId(), m_sourceFile, m_sourceFile, realFilename);

    // get path from script file name and add python path's
    // this is used for python so it will search modules from script path first
    pythonPath.emplace(scriptDir);

    // add all addon module dependencies to path
    if (m_addon)
    {
      std::set<std::string> paths;
      getAddonModuleDeps(m_addon, paths);
      for (const auto& it : paths)
        pythonPath.emplace(it);
    }
    else
    { // for backwards compatibility.
      // we don't have any addon so just add all addon modules installed
      CLog::Log(
          LOGWARNING,
          "CPythonInvoker({}): Script invoked without an addon. Adding all addon "
          "modules installed to python path as fallback. This behaviour will be removed in future "
          "version.",
          GetId());
      ADDON::VECADDONS addons;
      CServiceBroker::GetAddonMgr().GetAddons(addons, ADDON::AddonType::SCRIPT_MODULE);
      for (unsigned int i = 0; i < addons.size(); ++i)
        pythonPath.emplace(CSpecialProtocol::TranslatePath(addons[i]->LibPath()));
    }

    PyObject* sysPath = PySys_GetObject("path");

    std::for_each(pythonPath.crbegin(), pythonPath.crend(),
                  [&sysPath](const auto& path)
                  {
                    PyObject* pyPath = PyUnicode_FromString(path.c_str());
                    PyList_Insert(sysPath, 0, pyPath);

                    Py_DECREF(pyPath);
                  });

    CLog::Log(LOGDEBUG, "CPythonInvoker({}): full python path:", GetId());

    Py_ssize_t pathListSize = PyList_Size(sysPath);

    for (Py_ssize_t index = 0; index < pathListSize; index++)
    {
      if (index == 0 && !pythonPath.empty())
        CLog::Log(LOGDEBUG, "CPythonInvoker({}):   custom python path:", GetId());

      if (index == static_cast<ssize_t>(pythonPath.size()))
        CLog::Log(LOGDEBUG, "CPythonInvoker({}):   default python path:", GetId());

      PyObject* pyPath = PyList_GetItem(sysPath, index);
      CLog::Log(LOGDEBUG, "CPythonInvoker({}):     {}", GetId(), PyUnicode_AsUTF8(pyPath));
    }

    { // set the m_threadState to this new interp
      std::unique_lock<CCriticalSection> lockMe(m_critical);
      m_threadState = l_threadState;
    }
  }
  else
    // swap in my thread m_threadState
    PyThreadState_Swap(m_threadState);

  PyObject* sysArgv = PyList_New(0);

  if (arguments.empty())
    arguments.emplace_back(L"");

  CLog::Log(LOGDEBUG, "CPythonInvoker({}): adding args:", GetId());

  for (const auto& arg : arguments)
  {
    PyObject* pyArg = PyUnicode_FromWideChar(arg.c_str(), arg.length());
    PyList_Append(sysArgv, pyArg);
    CLog::Log(LOGDEBUG, "CPythonInvoker({}):  {}", GetId(), PyUnicode_AsUTF8(pyArg));

    Py_DECREF(pyArg);
  }

  PySys_SetObject("argv", sysArgv);
  Py_DECREF(sysArgv);

  CLog::Log(LOGDEBUG, "CPythonInvoker({}, {}): entering source directory {}", GetId(), m_sourceFile,
            scriptDir);
  PyObject* module = PyImport_AddModule("__main__");
  PyObject* moduleDict = PyModule_GetDict(module);

  // we need to check if we was asked to abort before we had inited
  bool stopping = false;
  {
    GilSafeSingleLock lock(m_critical);
    stopping = m_stop;
  }

  bool failed = false;
  std::string exceptionType, exceptionValue, exceptionTraceback;
  if (!stopping)
  {
    try
    {
      // run script from file
      // We need to have python open the file because on Windows the DLL that python
      //  is linked against may not be the DLL that xbmc is linked against so
      //  passing a FILE* to python from an fopen has the potential to crash.

      PyObject* pyRealFilename = Py_BuildValue("s", realFilename.c_str());
      FILE* fp = _Py_fopen_obj(pyRealFilename, "rb");
      Py_DECREF(pyRealFilename);

      if (fp != NULL)
      {
        PyObject* f = PyUnicode_FromString(realFilename.c_str());
        PyDict_SetItemString(moduleDict, "__file__", f);

        onPythonModuleInitialization(moduleDict);

        Py_DECREF(f);
        setState(InvokerStateRunning);
        XBMCAddon::Python::PyContext
            pycontext; // this is a guard class that marks this callstack as being in a python context
        executeScript(fp, realFilename, moduleDict);
      }
      else
        CLog::Log(LOGERROR, "CPythonInvoker({}, {}): {} not found!", GetId(), m_sourceFile,
                  m_sourceFile);
    }
    catch (const XbmcCommons::Exception& e)
    {
      setState(InvokerStateFailed);
      e.LogThrowMessage();
      failed = true;
    }
    catch (...)
    {
      setState(InvokerStateFailed);
      CLog::Log(LOGERROR, "CPythonInvoker({}, {}): failure in script", GetId(), m_sourceFile);
      failed = true;
    }
  }

  m_systemExitThrown = false;
  InvokerState stateToSet;
  if (!failed && !PyErr_Occurred())
  {
    CLog::Log(LOGDEBUG, "CPythonInvoker({}, {}): script successfully run", GetId(), m_sourceFile);
    stateToSet = InvokerStateScriptDone;
    onSuccess();
  }
  else if (PyErr_ExceptionMatches(PyExc_SystemExit))
  {
    m_systemExitThrown = true;
    CLog::Log(LOGDEBUG, "CPythonInvoker({}, {}): script aborted", GetId(), m_sourceFile);
    stateToSet = InvokerStateFailed;
    onAbort();
  }
  else
  {
    stateToSet = InvokerStateFailed;

    // if it failed with an exception we already logged the details
    if (!failed)
    {
      PythonBindings::PythonToCppException* e = NULL;
      if (PythonBindings::PythonToCppException::ParsePythonException(exceptionType, exceptionValue,
                                                                     exceptionTraceback))
        e = new PythonBindings::PythonToCppException(exceptionType, exceptionValue,
                                                     exceptionTraceback);
      else
        e = new PythonBindings::PythonToCppException();

      e->LogThrowMessage();
      delete e;
    }

    onError(exceptionType, exceptionValue, exceptionTraceback);
  }

  std::unique_lock<CCriticalSection> lock(m_critical);
  // no need to do anything else because the script has already stopped
  if (failed)
  {
    setState(stateToSet);
    return true;
  }

  if (m_threadState)
  {
    // make sure all sub threads have finished
    for (PyThreadState* old = nullptr; m_threadState != nullptr;)
    {
      PyThreadState* s = PyInterpreterState_ThreadHead(m_threadState->interp);
      for (; s && s == m_threadState;)
        s = PyThreadState_Next(s);

      if (!s)
        break;

      if (old != s)
      {
        CLog::Log(LOGINFO, "CPythonInvoker({}, {}): waiting on thread {}", GetId(), m_sourceFile,
                  (uint64_t)s->thread_id);
        old = s;
      }

      lock.unlock();
      CPyThreadState pyState;
      KODI::TIME::Sleep(100ms);
      pyState.Restore();
      lock.lock();
    }
  }

  // pending calls must be cleared out
  XBMCAddon::RetardedAsyncCallbackHandler::clearPendingCalls(m_threadState);

  assert(m_threadState != nullptr);
  PyEval_ReleaseThread(m_threadState);

  setState(stateToSet);

  return true;
}

void CPythonInvoker::executeScript(FILE* fp, const std::string& script, PyObject* moduleDict)
{
  if (fp == NULL || script.empty() || moduleDict == NULL)
    return;

  int m_Py_file_input = Py_file_input;
  PyRun_FileExFlags(fp, script.c_str(), m_Py_file_input, moduleDict, moduleDict, 1, NULL);
}

FILE* CPythonInvoker::PyFile_AsFileWithMode(PyObject* py_file, const char* mode)
{
  PyObject* ret = PyObject_CallMethod(py_file, "flush", "");
  if (ret == NULL)
    return NULL;
  Py_DECREF(ret);

  int fd = PyObject_AsFileDescriptor(py_file);
  if (fd == -1)
    return NULL;

  FILE* f = fdopen(fd, mode);
  if (f == NULL)
  {
    PyErr_SetFromErrno(PyExc_OSError);
    return NULL;
  }

  return f;
}

bool CPythonInvoker::stop(bool abort)
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  m_stop = true;

  if (!IsRunning() && !m_threadState)
    return false;

  if (m_threadState != NULL)
  {
    if (IsRunning())
    {
      setState(InvokerStateStopping);
      lock.unlock();

      PyEval_RestoreThread((PyThreadState*)m_threadState);

      //tell xbmc.Monitor to call onAbortRequested()
      if (m_addon)
      {
        CLog::Log(LOGDEBUG, "CPythonInvoker({}, {}): trigger Monitor abort request", GetId(),
                  m_sourceFile);
        AbortNotification();
      }

      PyEval_ReleaseThread(m_threadState);
    }
    else
      //Release the lock while waiting for threads to finish
      lock.unlock();

    XbmcThreads::EndTime<> timeout(PYTHON_SCRIPT_TIMEOUT);
    while (!m_stoppedEvent.Wait(15ms))
    {
      if (timeout.IsTimePast())
      {
        CLog::Log(LOGERROR,
                  "CPythonInvoker({}, {}): script didn't stop in {} seconds - let's kill it",
                  GetId(), m_sourceFile,
                  std::chrono::duration_cast<std::chrono::seconds>(PYTHON_SCRIPT_TIMEOUT).count());
        break;
      }

      // We can't empty-spin in the main thread and expect scripts to be able to
      // dismantle themselves. Python dialogs aren't normal XBMC dialogs, they rely
      // on TMSG_GUI_PYTHON_DIALOG messages, so pump the message loop.
      if (CServiceBroker::GetAppMessenger()->IsProcessThread())
      {
        CServiceBroker::GetAppMessenger()->ProcessMessages();
      }
    }

    lock.lock();

    setState(InvokerStateExecutionDone);

    // Useful for add-on performance metrics
    if (!timeout.IsTimePast())
      CLog::Log(LOGDEBUG, "CPythonInvoker({}, {}): script termination took {}ms", GetId(),
                m_sourceFile, (PYTHON_SCRIPT_TIMEOUT - timeout.GetTimeLeft()).count());

    // Since we released the m_critical it's possible that the state is cleaned up
    // so we need to recheck for m_threadState == NULL
    if (m_threadState != NULL)
    {
      {
        // grabbing the PyLock while holding the m_critical is asking for a deadlock
        CSingleExit ex2(m_critical);
        PyEval_RestoreThread((PyThreadState*)m_threadState);
      }


      PyThreadState* state = PyInterpreterState_ThreadHead(m_threadState->interp);
      while (state)
      {
        // Raise a SystemExit exception in python threads
        Py_XDECREF(state->async_exc);
        state->async_exc = PyExc_SystemExit;
        Py_XINCREF(state->async_exc);
        state = PyThreadState_Next(state);
      }

      // If a dialog entered its doModal(), we need to wake it to see the exception
      pulseGlobalEvent();

      PyEval_ReleaseThread(m_threadState);
    }
    lock.unlock();

    setState(InvokerStateFailed);
  }

  return true;
}

// Always called from Invoker thread
void CPythonInvoker::onExecutionDone()
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  if (m_threadState != NULL)
  {
    CLog::Log(LOGDEBUG, "{}({}, {})", __FUNCTION__, GetId(), m_sourceFile);

    PyEval_RestoreThread(m_threadState);

    onDeinitialization();

    // run the gc before finishing
    //
    // if the script exited by throwing a SystemExit exception then going back
    // into the interpreter causes this python bug to get hit:
    //    http://bugs.python.org/issue10582
    // and that causes major failures. So we are not going to go back in
    // to run the GC if that's the case.
    if (!m_stop && m_languageHook->HasRegisteredAddonClasses() && !m_systemExitThrown &&
        PyRun_SimpleString(GC_SCRIPT) == -1)
      CLog::Log(LOGERROR,
                "CPythonInvoker({}, {}): failed to run the gc to clean up after running prior to "
                "shutting down the Interpreter",
                GetId(), m_sourceFile);

    // PyErr_Clear() is required to prevent the debug python library to trigger an assert() at the Py_EndInterpreter() level
    PyErr_Clear();

    Py_EndInterpreter(m_threadState);

    // If we still have objects left around, produce an error message detailing what's been left behind
    if (m_languageHook->HasRegisteredAddonClasses())
      CLog::Log(LOGWARNING,
                "CPythonInvoker({}, {}): the python script \"{}\" has left several "
                "classes in memory that we couldn't clean up. The classes include: {}",
                GetId(), m_sourceFile, m_sourceFile, getListOfAddonClassesAsString(m_languageHook));

    // unregister the language hook
    m_languageHook->UnregisterMe();

#if PY_VERSION_HEX < 0x03070000
    PyEval_ReleaseLock();
#else
    PyThreadState_Swap(PyInterpreterState_ThreadHead(PyInterpreterState_Main()));
    PyEval_SaveThread();
#endif

    // set stopped event - this allows ::stop to run and kill remaining threads
    // this event has to be fired without holding m_critical
    // also the GIL (PyEval_AcquireLock) must not be held
    // if not obeyed there is still no deadlock because ::stop waits with timeout (smart one!)
    m_stoppedEvent.Set();

    m_threadState = nullptr;

    setState(InvokerStateExecutionDone);
  }
  ILanguageInvoker::onExecutionDone();
}

void CPythonInvoker::onExecutionFailed()
{
  PyEval_SaveThread();

  setState(InvokerStateFailed);
  CLog::Log(LOGERROR, "CPythonInvoker({}, {}): abnormally terminating python thread", GetId(),
            m_sourceFile);

  std::unique_lock<CCriticalSection> lock(m_critical);
  m_threadState = NULL;

  ILanguageInvoker::onExecutionFailed();
}

void CPythonInvoker::onInitialization()
{
  XBMC_TRACE;

  // get a possible initialization script
  const char* runscript = getInitializationScript();
  if (runscript != NULL && strlen(runscript) > 0)
  {
    // redirecting default output to debug console
    if (PyRun_SimpleString(runscript) == -1)
      CLog::Log(LOGFATAL, "CPythonInvoker({}, {}): initialize error", GetId(), m_sourceFile);
  }
}

void CPythonInvoker::onPythonModuleInitialization(void* moduleDict)
{
  if (m_addon.get() == NULL || moduleDict == NULL)
    return;

  PyObject* moduleDictionary = (PyObject*)moduleDict;

  PyDict_SetItemString(moduleDictionary, "__xbmcaddonid__",
                       PyObjectPtr(PyUnicode_FromString(m_addon->ID().c_str())).get());

  ADDON::CAddonVersion version = m_addon->GetDependencyVersion("xbmc.python");
  PyDict_SetItemString(moduleDictionary, "__xbmcapiversion__",
                       PyObjectPtr(PyUnicode_FromString(version.asString().c_str())).get());

  PyDict_SetItemString(moduleDictionary, "__xbmcinvokerid__", PyLong_FromLong(GetId()));

  CLog::Log(LOGDEBUG,
            "CPythonInvoker({}, {}): instantiating addon using automatically obtained id of \"{}\" "
            "dependent on version {} of the xbmc.python api",
            GetId(), m_sourceFile, m_addon->ID(), version.asString());
}

void CPythonInvoker::onDeinitialization()
{
  XBMC_TRACE;
}

void CPythonInvoker::onError(const std::string& exceptionType /* = "" */,
                             const std::string& exceptionValue /* = "" */,
                             const std::string& exceptionTraceback /* = "" */)
{
  CPyThreadState releaseGil;
  std::unique_lock<CCriticalSection> gc(CServiceBroker::GetWinSystem()->GetGfxContext());

  CGUIDialogKaiToast* pDlgToast =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogKaiToast>(
          WINDOW_DIALOG_KAI_TOAST);
  if (pDlgToast != NULL)
  {
    std::string message;
    if (m_addon && !m_addon->Name().empty())
      message = StringUtils::Format(g_localizeStrings.Get(2102), m_addon->Name());
    else
      message = g_localizeStrings.Get(2103);
    pDlgToast->QueueNotification(CGUIDialogKaiToast::Error, message, g_localizeStrings.Get(2104));
  }
}

void CPythonInvoker::getAddonModuleDeps(const ADDON::AddonPtr& addon, std::set<std::string>& paths)
{
  for (const auto& it : addon->GetDependencies())
  {
    //Check if dependency is a module addon
    ADDON::AddonPtr dependency;
    if (CServiceBroker::GetAddonMgr().GetAddon(it.id, dependency, ADDON::AddonType::SCRIPT_MODULE,
                                               ADDON::OnlyEnabled::CHOICE_YES))
    {
      std::string path = CSpecialProtocol::TranslatePath(dependency->LibPath());
      if (paths.find(path) == paths.end())
      {
        // add it and its dependencies
        paths.insert(path);
        getAddonModuleDeps(dependency, paths);
      }
    }
  }
}

void CPythonInvoker::PyObjectDeleter::operator()(PyObject* p) const
{
  assert(Py_REFCNT(p) == 2);
  Py_DECREF(p);
}
