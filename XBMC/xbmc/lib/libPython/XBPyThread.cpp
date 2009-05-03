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
#include "stdafx.h"
#include "Python/Include/Python.h"
#include "Python/Include/osdefs.h"
#include "XBPythonDll.h"
#include "FileSystem/SpecialProtocol.h"
#include "GUIWindowManager.h"
#include "GUIDialogOK.h"
	 
#include "XBPyThread.h"
#include "XBPython.h"

#ifndef __GNUC__
#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")
#endif

#ifdef _WIN32PC
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
  m_pExecuter = pExecuter;
  m_threadState = NULL;
  m_id = id;

  done = false;
  stopping = false;
  argv = NULL;
  source = NULL;
  argc = 0;
}

XBPyThread::~XBPyThread()
{
  StopThread();
  CLog::Log(LOGDEBUG,"python thread %d destructed", m_id);
  if (source) delete []source;
  if (argv)
  {
    for (unsigned int i = 0; i < argc; i++)
      delete [] argv[i];
    delete [] argv;
  }
}

int XBPyThread::evalFile(const char *src)
{
  type = 'F';
  source = new char[strlen(src)+1];
  strcpy(source, src);
  Create();
  return 0;
}

int XBPyThread::evalString(const char *src)
{
  type = 'S';
  source = new char[strlen(src)+1];
  strcpy(source, src);
  Create();
  return 0;
}

int XBPyThread::setArgv(unsigned int src_argc, const char **src)
{
  if (src == NULL)
    return 1;
  argc = src_argc;
  argv = new char*[argc];
  for(unsigned int i = 0; i < argc; i++)
  {
    argv[i] = new char[strlen(src[i])+1];
    strcpy(argv[i], src[i]);
  }
  return 0;
}

void XBPyThread::OnStartup(){}

void XBPyThread::Process()
{
  CLog::Log(LOGDEBUG,"Python thread: start processing");

  char path[1024];
  char sourcedir[1024];

  // get the global lock
  PyEval_AcquireLock();

  m_threadState = Py_NewInterpreter();
  
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();

  if (!m_threadState) {
    CLog::Log(LOGERROR,"Python thread: FAILED to get thread state!");
    return;
  }

  PyEval_AcquireLock();

  // swap in my thread state
  PyThreadState_Swap(m_threadState);

  m_pExecuter->InitializeInterpreter();
  
  CLog::Log(LOGDEBUG, "%s - The source file to load is %s", __FUNCTION__, source);

  // get path from script file name and add python path's
  // this is used for python so it will search modules from script path first
  strcpy(sourcedir, _P(source));

  char *p = strrchr(sourcedir, PATH_SEPARATOR_CHAR);
  *p = PY_PATH_SEP;
  *++p = 0;

  strcpy(path, sourcedir);

#ifndef _LINUX
  strcat(path, dll_getenv("PYTHONPATH"));
#else
  strcat(path, Py_GetPath());
#endif

  // set current directory and python's path.
  if (argv != NULL)
  {
    PySys_SetArgv(argc, argv);
  }

  CLog::Log(LOGDEBUG, "%s - Setting the Python path to %s", __FUNCTION__, path);

  PySys_SetPath(path);
  // Remove the PY_PATH_SEP at the end
  sourcedir[strlen(sourcedir)-1] = 0;
  
  CLog::Log(LOGDEBUG, "%s - Entering source directory %s", __FUNCTION__, sourcedir);
  
  xbp_chdir(sourcedir);
  
  if (type == 'F')
  {
    // run script from file
    FILE *fp = fopen_utf8(_P(source).c_str(), "r");
    if (fp)
    {
      if (PyRun_SimpleFile(fp, _P(source).c_str()) == -1)
      {
        CLog::Log(LOGERROR, "Scriptresult: Error\n");
        if (PyErr_Occurred()) PyErr_Print();
        
        CGUIDialogOK *pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
        if (pDlgOK)
        {
          // TODO: Need to localize this
          pDlgOK->SetHeading(247); //Scripts
          pDlgOK->SetLine(0, 257); //ERROR
          pDlgOK->SetLine(1, "Python script failed:");
          pDlgOK->SetLine(2, source);
          pDlgOK->DoModal();
        }
      }
      else CLog::Log(LOGINFO, "Scriptresult: Succes\n");
      fclose(fp);
    }
    else CLog::Log(LOGERROR, "%s not found!\n", source);
  }
  else
  {
    //run script
    if (PyRun_SimpleString(source) == -1)
    {
      CLog::Log(LOGERROR, "Scriptresult: Error\n");
      if (PyErr_Occurred()) PyErr_Print();
     
      CGUIDialogOK *pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (pDlgOK)
      {
        // TODO: Need to localize this
        pDlgOK->SetHeading(247); //Scripts
        pDlgOK->SetLine(0, 257); //ERROR
        pDlgOK->SetLine(1, "Python script failed:");
        pDlgOK->SetLine(2, source);
        pDlgOK->DoModal();
      }
    }
    else CLog::Log(LOGINFO, "Scriptresult: Success\n");
  }

  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();

  // when a script uses threads or timers - we have to wait for them to be over before we terminate the interpreter.
  // so first - release the lock and allow the threads to terminate.

  ::Sleep(500);
  PyEval_AcquireLock();

  PyThreadState_Swap(m_threadState);

  // look waiting for the running threads to end
  PyRun_SimpleString(
        "import threading\n"
        "import sys\n"
        "try:\n"
            "\tthreads = list(threading.enumerate())\n"
        "except:\n"
            "\tprint 'error listing threads'\n"
        "while threading.activeCount() > 1:\n"
        "\tfor thread in threads:\n"
            "\t\tif thread <> threading.currentThread():\n"
            "\t\t\tprint 'waiting for thread - ' + thread.getName()\n"
            "\t\t\tthread.join(1000)\n"
        );

  m_pExecuter->DeInitializeInterpreter();

  Py_EndInterpreter(m_threadState);
  m_threadState = NULL;
  PyEval_ReleaseLock();
}

void XBPyThread::OnExit()
{
  done = true;
  m_pExecuter->setDone(m_id);
}

void XBPyThread::OnException()
{
  done = true;
  m_threadState = NULL;
  CLog::Log(LOGERROR,"%s, abnormally terminating python thread", __FUNCTION__);
  PyEval_ReleaseLock();
  m_pExecuter->setDone(m_id);
}

bool XBPyThread::isDone() {
  return done;
}

bool XBPyThread::isStopping() {
  return stopping;
}

void XBPyThread::stop()
{
  if (m_threadState)
  {
    PyEval_AcquireLock();
    m_threadState->c_tracefunc = xbTrace;
    m_threadState->use_tracing = 1;
    PyEval_ReleaseLock();
  }

  stopping = true;
}
