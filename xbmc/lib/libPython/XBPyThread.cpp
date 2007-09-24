
// python.h should always be included first before any other includes
#include "stdafx.h"
#ifndef _LINUX
#include "python/Python.h"
#else
#include <python2.4/Python.h>
#include "XBPythonDll.h"
#endif
#include "Util.h"

#include "XBPyThread.h"
#include "XBPython.h"

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

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
  PyEval_ReleaseLock();

  if (!m_threadState) {
	CLog::Log(LOGERROR,"Python thread: FAILED to get thread state!");
	return;
  }

  PyEval_AcquireLock();

  // swap in my thread state
  PyThreadState_Swap(m_threadState);

  m_pExecuter->InitializeInterpreter();

  // get path from script file name and add python path's
  // this is used for python so it will search modules from script path first
  strcpy(sourcedir, source);
#ifndef _LINUX
  strcpy(strrchr(sourcedir, PATH_SEPARATOR_CHAR), ";");
#else
  strcpy(strrchr(sourcedir, PATH_SEPARATOR_CHAR), ":");
#endif

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
  PySys_SetPath(path);

#ifdef _LINUX
  // Replace the : at the end with ; so it will be EXACTLY like the xbox version
  strcpy(strrchr(sourcedir, ':'), ";");
#endif  
  xbp_chdir(sourcedir); // XXX, there is a ';' at the end

  if (type == 'F')
  {
    // run script from file
    FILE *fp = fopen(source, "r");
    if (fp)
    {
      if (PyRun_SimpleFile(fp, source) == -1)
      {
        CLog::Log(LOGERROR, "Scriptresult: Error\n");
        if (PyErr_Occurred())	PyErr_Print();
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
    }
    else CLog::Log(LOGINFO, "Scriptresult: Success\n");
  }

  m_pExecuter->DeInitializeInterpreter();

  // clear the thread state and release our hold on the global interpreter
  Py_EndInterpreter(m_threadState);
  m_threadState = NULL;
  PyEval_ReleaseLock();
}

void XBPyThread::OnExit()
{
  done = true;
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
