/*
 *      Copyright (C) 2005-2009 Team XBMC
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

// python.h should always be included first before any other includes
#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#if (defined USE_EXTERNAL_PYTHON)
  #if (defined HAVE_LIBPYTHON2_6)
    #include <python2.6/Python.h>
    #include <python2.6/osdefs.h>
  #elif (defined HAVE_LIBPYTHON2_5)
    #include <python2.5/Python.h>
    #include <python2.5/osdefs.h>
  #elif (defined HAVE_LIBPYTHON2_4)
    #include <python2.4/Python.h>
    #include <python2.4/osdefs.h>
  #else
    #error "Could not determine version of Python to use."
  #endif
#else
  #include "Python/Include/Python.h"
  #include "Python/Include/osdefs.h"
#endif
#include "XBPythonDll.h"
#include "FileSystem/SpecialProtocol.h"
#include "GUIWindowManager.h"
#include "GUIDialogKaiToast.h"
#include "LocalizeStrings.h"
#include "utils/log.h"
#include "utils/SingleLock.h"
#include "Util.h"
#include "addons/AddonManager.h"

#include "XBPyThread.h"
#include "XBPython.h"

#include "lib/libPython/xbmcmodule/pyutil.h"

#ifndef __GNUC__
#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")
#endif

#ifdef _WIN32
extern "C" FILE *fopen_utf8(const char *_Filename, const char *_Mode);
#else
#define fopen_utf8 fopen
#endif

#define PY_PATH_SEP DELIM

extern "C"
{
  int xbp_chdir(const char *dirname);
  char* dll_getenv(const char* szKey);
}

int xbTrace(PyObject *obj, _frame *frame, int what, PyObject *arg)
{
  PyErr_SetString(PyExc_KeyboardInterrupt, "script interrupted by user\n");
  return -1;
}

XBPyThread::XBPyThread(XBPython *pExecuter, int id)
{
  CLog::Log(LOGDEBUG,"new python thread created. id=%d", id);
  m_pExecuter   = pExecuter;
  m_threadState = NULL;
  m_id          = id;
  m_stopping    = false;
  m_argv        = NULL;
  m_source      = NULL;
  m_argc        = 0;
}

XBPyThread::~XBPyThread()
{
  stop();
  g_pythonParser.PulseGlobalEvent();
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

int XBPyThread::evalFile(const CStdString &src)
{
  m_type    = 'F';
  m_source  = new char[src.GetLength()+1];
  strcpy(m_source, src);
  Create();
  return 0;
}

int XBPyThread::evalString(const CStdString &src)
{
  m_type    = 'S';
  m_source  = new char[src.GetLength()+1];
  strcpy(m_source, src);
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

void XBPyThread::OnStartup()
{
  CThread::SetName("Python Thread");
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

  m_pExecuter->InitializeInterpreter();

  CLog::Log(LOGDEBUG, "%s - The source file to load is %s", __FUNCTION__, m_source);

  // get path from script file name and add python path's
  // this is used for python so it will search modules from script path first
  CStdString scriptDir;
  CUtil::GetDirectory(_P(m_source), scriptDir);
  CUtil::RemoveSlashAtEnd(scriptDir);
  CStdString path = scriptDir;

  // add on any addon modules the user has installed
  ADDON::VECADDONS addons;
  ADDON::CAddonMgr::Get().GetAddons(ADDON::ADDON_SCRIPT_MODULE, addons);
  for (unsigned int i = 0; i < addons.size(); ++i)
    path += PY_PATH_SEP + _P(addons[i]->LibPath());

  // and add on whatever our default path is
  path += PY_PATH_SEP;
  path += Py_GetPath();

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

  xbp_chdir(scriptDir.c_str());

  if (!stopping)
  {
    if (m_type == 'F')
    {
      // run script from file
      FILE *fp = fopen_utf8(_P(m_source).c_str(), "r");
      if (fp)
      {
        PyObject *f = PyString_FromString(_P(m_source).c_str());
        PyDict_SetItemString(moduleDict, "__file__", f);
        Py_DECREF(f);
        PyRun_File(fp, _P(m_source).c_str(), m_Py_file_input, moduleDict, moduleDict);
        fclose(fp);
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

  if (PyErr_Occurred())
  {
    PyObject* exc_type;
    PyObject* exc_value;
    PyObject* exc_traceback;
    PyObject* pystring;
    pystring = NULL;

    PyErr_Fetch(&exc_type, &exc_value, &exc_traceback);
    if (exc_type == 0 && exc_value == 0 && exc_traceback == 0)
    {
      CLog::Log(LOGINFO, "Strange: No Python exception occured");
    }
    else
    {
      if (exc_type != NULL && (pystring = PyObject_Str(exc_type)) != NULL && (PyString_Check(pystring)))
      {
        if (strncmp(PyString_AsString(pystring), "exceptions.KeyboardInterrupt", 28) == 0)
          CLog::Log(LOGINFO, "Scriptresult: Interrupted by user");
        else
        {
          PyObject *tracebackModule;

          CLog::Log(LOGINFO, "-->Python script returned the following error<--");
          CLog::Log(LOGERROR, "Error Type: %s", PyString_AsString(PyObject_Str(exc_type)));
          if (PyObject_Str(exc_value))
            CLog::Log(LOGERROR, "Error Contents: %s", PyString_AsString(PyObject_Str(exc_value)));

          tracebackModule = PyImport_ImportModule((char*)"traceback");
          if (tracebackModule != NULL)
          {
            PyObject *tbList, *emptyString, *strRetval;

            tbList = PyObject_CallMethod(tracebackModule, (char*)"format_exception", (char*)"OOO", exc_type, exc_value == NULL ? Py_None : exc_value, exc_traceback == NULL ? Py_None : exc_traceback);
            emptyString = PyString_FromString("");
            strRetval = PyObject_CallMethod(emptyString, (char*)"join", (char*)"O", tbList);

            CLog::Log(LOGERROR, "%s", PyString_AsString(strRetval));

            Py_DECREF(tbList);
            Py_DECREF(emptyString);
            Py_DECREF(strRetval);
            Py_DECREF(tracebackModule);
          }
          CLog::Log(LOGINFO, "-->End of Python script error report<--");
        }
      }
      else
      {
        pystring = NULL;
        CLog::Log(LOGINFO, "<unknown exception type>");
      }
    }
    if (pystring != NULL && strncmp(PyString_AsString(pystring), "exceptions.KeyboardInterrupt", 28) != 0)
    {
      CGUIDialogKaiToast *pDlgToast = (CGUIDialogKaiToast*)g_windowManager.GetWindow(WINDOW_DIALOG_KAI_TOAST);
      if (pDlgToast)
      {
        CStdString desc;
        CStdString path;
        CStdString script;
        CUtil::Split(m_source, path, script);
        if (script.Equals("default.py"))
        {
          CStdString path2;
          CUtil::RemoveSlashAtEnd(path);
          CUtil::Split(path, path2, script);
        }

        desc.Format(g_localizeStrings.Get(2100), script);
        pDlgToast->QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(257), desc);
      }
    }
    Py_XDECREF(exc_type);
    Py_XDECREF(exc_value); // caller owns all 3
    Py_XDECREF(exc_traceback); // already NULL'd out
    Py_XDECREF(pystring);
  }
  else
    CLog::Log(LOGINFO, "Scriptresult: Success");

  // wait for the running threads to end
  PyErr_Clear();
  PyRun_SimpleString(
        "import threading\n"
        "import sys\n"
        "while threading.activeCount() > 1:\n"
        "\tthreads = list(threading.enumerate())\n"
        "\tfor thread in threads:\n"
        "\t\tif thread <> threading.currentThread():\n"
        "\t\t\tprint 'waiting for thread - ' + thread.getName()\n"
        "\t\t\tthread.join(1000)\n"
        );

  if (PyErr_Occurred())
    CLog::Log(LOGERROR, "Failed to wait for python threads to end");

  // pending calls must be cleared out
  PyXBMC_ClearPendingCalls(state);

  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();

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
    PyThreadState* old = PyThreadState_Swap(m_threadState);
    PyRun_SimpleString("import xbmc\n"
                       "xbmc.abortRequested = True\n");

    m_threadState->c_tracefunc = xbTrace;
    m_threadState->use_tracing = 1;

    PyThreadState_Swap(old);
    PyEval_ReleaseLock();
  }
}
