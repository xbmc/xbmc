/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif

// python.h should always be included first before any other includes
#include <Python.h>
#include <osdefs.h>

#include "system.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "utils/URIUtils.h"
#include "addons/AddonManager.h"
#include "addons/Addon.h"
#include "Application.h"
#include "ApplicationMessenger.h"

#include "XBPyThread.h"
#include "XBPython.h"

#include "interfaces/legacy/Exception.h"
#include "interfaces/legacy/CallbackHandler.h"
#include "interfaces/legacy/AddonUtils.h"
#include "interfaces/legacy/ModuleXbmc.h"

#include "interfaces/python/pythreadstate.h"
#include "interfaces/python/swig.h"
#include "utils/CharsetConverter.h"


#ifdef _WIN32
extern "C" FILE *fopen_utf8(const char *_Filename, const char *_Mode);
#else
#define fopen_utf8 fopen
#endif

#define PY_PATH_SEP DELIM

// Time before ill-behaved scripts are terminated
#define PYTHON_SCRIPT_TIMEOUT 5000 // ms

extern "C"
{
  int xbp_chdir(const char *dirname);
  char* dll_getenv(const char* szKey);
}

XBPyThread::XBPyThread(XBPython *pExecuter, int id) : CThread("XBPyThread")
{
  CLog::Log(LOGDEBUG,"new python thread created. id=%d", id);
  m_pExecuter   = pExecuter;
  m_threadState = NULL;
  m_id          = id;
  m_stopping    = false;
  m_argv        = NULL;
  m_source      = NULL;
  m_argc        = 0;
  m_type        = 0;
}

XBPyThread::~XBPyThread()
{
  stop();
  g_pythonParser.PulseGlobalEvent();
  CLog::Log(LOGDEBUG,"waiting for python thread %d to stop", m_id);
  StopThread();
  CLog::Log(LOGDEBUG,"python thread %d destructed", m_id);
  delete [] m_source;
  if (m_argv)
  {
    for (unsigned int i = 0; i < m_argc; i++)
      delete [] m_argv[i];
    delete [] m_argv;
  }
}

void XBPyThread::setSource(const CStdString &src)
{
#ifdef TARGET_WINDOWS
  CStdString strsrc = src;
  g_charsetConverter.utf8ToSystem(strsrc);
  m_source  = new char[strsrc.GetLength()+1];
  strcpy(m_source, strsrc);
#else
  m_source  = new char[src.GetLength()+1];
  strcpy(m_source, src);
#endif
}

int XBPyThread::evalFile(const CStdString &src)
{
  m_type    = 'F';
  setSource(src);
  Create();
  return 0;
}

int XBPyThread::evalString(const CStdString &src)
{
  m_type    = 'S';
  setSource(src);
  Create();
  return 0;
}

int XBPyThread::setArgv(const std::vector<CStdString> &argv)
{
  m_argc = argv.size();
  m_argv = new char*[m_argc];
  for(unsigned int i = 0; i < m_argc; i++)
  {
    m_argv[i] = new char[argv[i].GetLength()+1];
    strcpy(m_argv[i], argv[i].c_str());
  }
  return 0;
}

void XBPyThread::Process()
{
  CLog::Log(LOGDEBUG,"Python thread: start processing");

  int m_Py_file_input = Py_file_input;

  // get the global lock
  PyEval_AcquireLock();
  PyThreadState* state = Py_NewInterpreter();
  if (!state)
  {
    PyEval_ReleaseLock();
    CLog::Log(LOGERROR,"Python thread: FAILED to get thread state!");
    return;
  }
  // swap in my thread state
  PyThreadState_Swap(state);

  m_pExecuter->InitializeInterpreter(addon);

  CLog::Log(LOGDEBUG, "%s - The source file to load is %s", __FUNCTION__, m_source);

  // get path from script file name and add python path's
  // this is used for python so it will search modules from script path first
  CStdString scriptDir;
  URIUtils::GetDirectory(CSpecialProtocol::TranslatePath(m_source), scriptDir);
  URIUtils::RemoveSlashAtEnd(scriptDir);
  CStdString path = scriptDir;

  // add on any addon modules the user has installed
  ADDON::VECADDONS addons;
  ADDON::CAddonMgr::Get().GetAddons(ADDON::ADDON_SCRIPT_MODULE, addons);
  for (unsigned int i = 0; i < addons.size(); ++i)
#ifdef TARGET_WINDOWS
  {
    CStdString strTmp(CSpecialProtocol::TranslatePath(addons[i]->LibPath()));
    g_charsetConverter.utf8ToSystem(strTmp);
    path += PY_PATH_SEP + strTmp;
  }
#else
    path += PY_PATH_SEP + CSpecialProtocol::TranslatePath(addons[i]->LibPath());
#endif

  // and add on whatever our default path is
  path += PY_PATH_SEP;

  // we want to use sys.path so it includes site-packages
  // if this fails, default to using Py_GetPath
  PyObject *sysMod(PyImport_ImportModule((char*)"sys")); // must call Py_DECREF when finished
  PyObject *sysModDict(PyModule_GetDict(sysMod)); // borrowed ref, no need to delete
  PyObject *pathObj(PyDict_GetItemString(sysModDict, "path")); // borrowed ref, no need to delete

  if( pathObj && PyList_Check(pathObj) )
  {
    for( int i = 0; i < PyList_Size(pathObj); i++ )
    {
      PyObject *e = PyList_GetItem(pathObj, i); // borrowed ref, no need to delete
      if( e && PyString_Check(e) )
      {
        path += PyString_AsString(e); // returns internal data, don't delete or modify
        path += PY_PATH_SEP;
      }
    }
  }
  else
  {
    path += Py_GetPath();
  }
  Py_DECREF(sysMod); // release ref to sysMod

  // set current directory and python's path.
  if (m_argv != NULL)
    PySys_SetArgv(m_argc, m_argv);

  CLog::Log(LOGDEBUG, "%s - Setting the Python path to %s", __FUNCTION__, path.c_str());

  PySys_SetPath((char *)path.c_str());

  CLog::Log(LOGDEBUG, "%s - Entering source directory %s", __FUNCTION__, scriptDir.c_str());

  PyObject* module = PyImport_AddModule((char*)"__main__");
  PyObject* moduleDict = PyModule_GetDict(module);

  // when we are done initing we store thread state so we can be aborted
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();

  // we need to check if we was asked to abort before we had inited
  bool stopping = false;
  { CSingleLock lock(m_pExecuter->m_critSection);
    m_threadState = state;
    stopping = m_stopping;
  }

  PyEval_AcquireLock();
  PyThreadState_Swap(state);

  if (!stopping)
  {
    try
    {
    if (m_type == 'F')
    {
      // run script from file
      // We need to have python open the file because on Windows the DLL that python
      //  is linked against may not be the DLL that xbmc is linked against so
      //  passing a FILE* to python from an fopen has the potential to crash.
      PyObject* file = PyFile_FromString((char *) CSpecialProtocol::TranslatePath(m_source).c_str(), (char*)"r");
      FILE *fp = PyFile_AsFile(file);

      if (fp)
      {
        PyObject *f = PyString_FromString(CSpecialProtocol::TranslatePath(m_source).c_str());
        PyDict_SetItemString(moduleDict, "__file__", f);
        if (addon.get() != NULL)
        {
          PyObject *pyaddonid = PyString_FromString(addon->ID().c_str());
          PyDict_SetItemString(moduleDict, "__xbmcaddonid__", pyaddonid);

          CStdString version = ADDON::GetXbmcApiVersionDependency(addon);
          PyObject *pyxbmcapiversion = PyString_FromString(version.c_str());
          PyDict_SetItemString(moduleDict, "__xbmcapiversion__", pyxbmcapiversion);

          CLog::Log(LOGDEBUG,"Instantiating addon using automatically obtained id of \"%s\" dependent on version %s of the xbmc.python api",addon->ID().c_str(),version.c_str());
        }
        Py_DECREF(f);
        PyRun_FileExFlags(fp, CSpecialProtocol::TranslatePath(m_source).c_str(), m_Py_file_input, moduleDict, moduleDict,1,NULL);
      }
      else
        CLog::Log(LOGERROR, "%s not found!", m_source);
    }
    else
    {
      //run script
      PyRun_String(m_source, m_Py_file_input, moduleDict, moduleDict);
    }
    }
    catch (const XbmcCommons::Exception& e)
    {
      e.LogThrowMessage();
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "failure in %s", m_source);
    }
  }

  if (!PyErr_Occurred())
    CLog::Log(LOGINFO, "Scriptresult: Success");
  else if (PyErr_ExceptionMatches(PyExc_SystemExit))
    CLog::Log(LOGINFO, "Scriptresult: Aborted");
  else
  {
    PythonBindings::PythonToCppException e;
    e.LogThrowMessage();

    {
      CPyThreadState releaseGil;
      CSingleLock gc(g_graphicsContext);

      CGUIDialogKaiToast *pDlgToast = (CGUIDialogKaiToast*)g_windowManager.GetWindow(WINDOW_DIALOG_KAI_TOAST);
      if (pDlgToast)
      {
        CStdString desc;
        CStdString path;
        CStdString script;
        URIUtils::Split(m_source, path, script);
        if (script.Equals("default.py"))
        {
          CStdString path2;
          URIUtils::RemoveSlashAtEnd(path);
          URIUtils::Split(path, path2, script);
        }

        desc.Format(g_localizeStrings.Get(2100), script);
        pDlgToast->QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(257), desc);
      }
    }
  }

  PyObject *m = PyImport_AddModule((char*)"xbmc");
  if(!m || PyObject_SetAttrString(m, (char*)"abortRequested", PyBool_FromLong(1)))
    CLog::Log(LOGERROR, "Scriptresult: failed to set abortRequested");

  // make sure all sub threads have finished
  for(PyThreadState* s = state->interp->tstate_head, *old = NULL; s;)
  {
    if(s == state)
    {
      s = s->next;
      continue;
    }
    if(old != s)
    {
      CLog::Log(LOGINFO, "Scriptresult: Waiting on thread %"PRIu64, (uint64_t)s->thread_id);
      old = s;
    }

    CPyThreadState pyState;
    Sleep(100);
    pyState.Restore();

    s = state->interp->tstate_head;
  }

  // pending calls must be cleared out
  XBMCAddon::RetardedAsynchCallbackHandler::clearPendingCalls(state);

  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();

  //set stopped event - this allows ::stop to run and kill remaining threads
  //this event has to be fired without holding m_pExecuter->m_critSection
  //before
  //Also the GIL (PyEval_AcquireLock) must not be held
  //if not obeyed there is still no deadlock because ::stop waits with timeout (smart one!)
  stoppedEvent.Set();

  { CSingleLock lock(m_pExecuter->m_critSection);
    m_threadState = NULL;
  }

  PyEval_AcquireLock();
  PyThreadState_Swap(state);

  m_pExecuter->DeInitializeInterpreter();

  Py_EndInterpreter(state);
  PyThreadState_Swap(NULL);

  PyEval_ReleaseLock();
}

void XBPyThread::OnExit()
{
  m_pExecuter->setDone(m_id);
}

void XBPyThread::OnException()
{
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();

  CSingleLock lock(m_pExecuter->m_critSection);
  m_threadState = NULL;
  CLog::Log(LOGERROR,"%s, abnormally terminating python thread", __FUNCTION__);
  m_pExecuter->setDone(m_id);
}

bool XBPyThread::isStopping() {
  return m_stopping;
}

void XBPyThread::stop()
{
  CSingleLock lock(m_pExecuter->m_critSection);
  if(m_stopping)
    return;

  m_stopping = true;

  if (m_threadState)
  {
    PyEval_AcquireLock();
    PyThreadState* old = PyThreadState_Swap((PyThreadState*)m_threadState);

    //tell xbmc.Monitor to call onAbortRequested()
    if (addon)
      g_pythonParser.OnAbortRequested(addon->ID());

    PyObject *m;
    m = PyImport_AddModule((char*)"xbmc");
    if(!m || PyObject_SetAttrString(m, (char*)"abortRequested", PyBool_FromLong(1)))
      CLog::Log(LOGERROR, "XBPyThread::stop - failed to set abortRequested");

    PyThreadState_Swap(old);
    old = NULL;
    PyEval_ReleaseLock();

    XbmcThreads::EndTime timeout(PYTHON_SCRIPT_TIMEOUT);
    while (!stoppedEvent.WaitMSec(15))
    {
      if (timeout.IsTimePast())
      {
        CLog::Log(LOGERROR, "XBPyThread::stop - script didn't stop in %d seconds - let's kill it", PYTHON_SCRIPT_TIMEOUT / 1000);
        break;
      }
      // We can't empty-spin in the main thread and expect scripts to be able to
      // dismantle themselves. Python dialogs aren't normal XBMC dialogs, they rely
      // on TMSG_GUI_PYTHON_DIALOG messages, so pump the message loop.
      if (g_application.IsCurrentThread())
      {
        CSingleExit ex(g_graphicsContext);
        CApplicationMessenger::Get().ProcessMessages();
      }
    }
    // Useful for add-on performance metrics
    if (!timeout.IsTimePast())
      CLog::Log(LOGDEBUG, "XBPyThread::stop - script termination took %dms", PYTHON_SCRIPT_TIMEOUT - timeout.MillisLeft());
    
    //everything which didn't exit by now gets killed
    {
      // grabbing the PyLock while holding the XBPython m_critSection is asking for a deadlock
      CSingleExit ex2(m_pExecuter->m_critSection);
      PyEval_AcquireLock();
    }

    // since we released the XBPython m_critSection it's possible that the state is cleaned up 
    // so we need to recheck for m_threadState == NULL
    if (m_threadState)
    {
      old = PyThreadState_Swap((PyThreadState*)m_threadState);    
      for(PyThreadState* state = ((PyThreadState*)m_threadState)->interp->tstate_head; state; state = state->next)
      {
        // Raise a SystemExit exception in python threads
        Py_XDECREF(state->async_exc);
        state->async_exc = PyExc_SystemExit;
        Py_XINCREF(state->async_exc);
      }

      // If a dialog entered its doModal(), we need to wake it to see the exception
      g_pythonParser.PulseGlobalEvent();

    }

    if (old != NULL)
      PyThreadState_Swap(old);

    lock.Leave();
    PyEval_ReleaseLock();
  }
}
