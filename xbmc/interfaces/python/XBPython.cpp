/*
 *      Copyright (C) 2005-2008 Team XBMC
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
  #elif (defined HAVE_LIBPYTHON2_5)
    #include <python2.5/Python.h>
  #elif (defined HAVE_LIBPYTHON2_4)
    #include <python2.4/Python.h>
  #else
    #error "Could not determine version of Python to use."
  #endif
#else
  #include "python/Include/Python.h"
#endif
#include "cores/DllLoader/DllLoaderContainer.h"
#include "GUIPassword.h"

#include "XBPython.h"
#include "XBPythonDll.h"
#include "settings/Settings.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "utils/TimeUtils.h"

#ifndef _LINUX
#if (defined USE_EXTERNAL_PYTHON) && (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmcbin/system/python/python26.dll"
#else
#define PYTHON_DLL "special://xbmcbin/system/python/python24.dll"
#endif
#else
#if defined(__APPLE__)
  #if defined(__POWERPC__)
    #define PYTHON_DLL "special://xbmcbin/system/python/python24-powerpc-osx.so"
  #else
    #if (defined HAVE_LIBPYTHON2_6)
      #define PYTHON_DLL "special://xbmcbin/system/python/python26-x86-osx.so"
    #elif (defined HAVE_LIBPYTHON2_5)
      #define PYTHON_DLL "special://xbmcbin/system/python/python25-x86-osx.so"
    #else
      #define PYTHON_DLL "special://xbmcbin/system/python/python24-x86-osx.so"
    #endif
  #endif
#elif defined(__x86_64__)
#if (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmcbin/system/python/python26-x86_64-linux.so"
#elif (defined HAVE_LIBPYTHON2_5)
#define PYTHON_DLL "special://xbmcbin/system/python/python25-x86_64-linux.so"
#else
#define PYTHON_DLL "special://xbmcbin/system/python/python24-x86_64-linux.so"
#endif
#elif defined(_POWERPC)
#if (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmcbin/system/python/python26-powerpc-linux.so"
#elif (defined HAVE_LIBPYTHON2_5)
#define PYTHON_DLL "special://xbmcbin/system/python/python25-powerpc-linux.so"
#else
#define PYTHON_DLL "special://xbmcbin/system/python/python24-powerpc-linux.so"
#endif
#elif defined(_POWERPC64)
#if (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmcbin/system/python/python26-powerpc64-linux.so"
#elif (defined HAVE_LIBPYTHON2_5)
#define PYTHON_DLL "special://xbmcbin/system/python/python25-powerpc64-linux.so"
#else
#define PYTHON_DLL "special://xbmcbin/system/python/python24-powerpc64-linux.so"
#endif
#elif defined(_ARMEL)
#if (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmc/system/python/python26-arm.so"
#elif (defined HAVE_LIBPYTHON2_5)
#define PYTHON_DLL "special://xbmc/system/python/python25-arm.so"
#else
#define PYTHON_DLL "special://xbmc/system/python/python24-arm.so"
#endif
#else /* !__x86_64__ && !__powerpc__ */
#if (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmcbin/system/python/python26-i486-linux.so"
#elif (defined HAVE_LIBPYTHON2_5)
#define PYTHON_DLL "special://xbmcbin/system/python/python25-i486-linux.so"
#else
#define PYTHON_DLL "special://xbmcbin/system/python/python24-i486-linux.so"
#endif
#endif /* __x86_64__ */
#endif /* _LINUX */

extern "C" HMODULE __stdcall dllLoadLibraryA(LPCSTR file);
extern "C" BOOL __stdcall dllFreeLibrary(HINSTANCE hLibModule);

extern "C" {
  void InitXBMCModule(void);
  void InitXBMCTypes(void);
  void DeinitXBMCModule(void);
  void InitPluginModule(void);
  void InitPluginTypes(void);
  void DeinitPluginModule(void);
  void InitGUIModule(void);
  void InitGUITypes(void);
  void DeinitGUIModule(void);
  void InitAddonModule(void);
  void InitAddonTypes(void);
  void DeinitAddonModule(void);
  void InitVFSModule(void);
  void DeinitVFSModule(void);
}

XBPython::XBPython()
{
  m_bInitialized      = false;
  m_bLogin            = false;
  m_nextid            = 0;
  m_mainThreadState   = NULL;
  m_globalEvent       = CreateEvent(NULL, false, false, (char*)"pythonGlobalEvent");
  m_ThreadId          = CThread::GetCurrentThreadId();
  m_iDllScriptCounter = 0;
  m_vecPlayerCallbackList.clear();
}

XBPython::~XBPython()
{
  CloseHandle(m_globalEvent);
}

// message all registered callbacks that xbmc stopped playing
void XBPython::OnPlayBackEnded()
{
  CSingleLock lock(m_critSection);
  if (m_bInitialized)
  {
    PlayerCallbackList::iterator it = m_vecPlayerCallbackList.begin();
    while (it != m_vecPlayerCallbackList.end())
    {
      ((IPlayerCallback*)(*it))->OnPlayBackEnded();
      it++;
    }
  }
}

// message all registered callbacks that we started playing
void XBPython::OnPlayBackStarted()
{
  CSingleLock lock(m_critSection);
  if (m_bInitialized)
  {
    PlayerCallbackList::iterator it = m_vecPlayerCallbackList.begin();
    while (it != m_vecPlayerCallbackList.end())
    {
      ((IPlayerCallback*)(*it))->OnPlayBackStarted();
      it++;
    }
  }
}

// message all registered callbacks that we paused playing
void XBPython::OnPlayBackPaused()
{
  CSingleLock lock(m_critSection);
  if (m_bInitialized)
  {
    PlayerCallbackList::iterator it = m_vecPlayerCallbackList.begin();
    while (it != m_vecPlayerCallbackList.end())
    {
      ((IPlayerCallback*)(*it))->OnPlayBackPaused();
      it++;
    }
  }
}

// message all registered callbacks that we resumed playing
void XBPython::OnPlayBackResumed()
{
  CSingleLock lock(m_critSection);
  if (m_bInitialized)
  {
    PlayerCallbackList::iterator it = m_vecPlayerCallbackList.begin();
    while (it != m_vecPlayerCallbackList.end())
    {
      ((IPlayerCallback*)(*it))->OnPlayBackResumed();
      it++;
    }
  }
}

// message all registered callbacks that user stopped playing
void XBPython::OnPlayBackStopped()
{
  CSingleLock lock(m_critSection);
  if (m_bInitialized)
  {
    PlayerCallbackList::iterator it = m_vecPlayerCallbackList.begin();
    while (it != m_vecPlayerCallbackList.end())
    {
      ((IPlayerCallback*)(*it))->OnPlayBackStopped();
      it++;
    }
  }
}

void XBPython::RegisterPythonPlayerCallBack(IPlayerCallback* pCallback)
{
  CSingleLock lock(m_critSection);
  m_vecPlayerCallbackList.push_back(pCallback);
}

void XBPython::UnregisterPythonPlayerCallBack(IPlayerCallback* pCallback)
{
  CSingleLock lock(m_critSection);
  PlayerCallbackList::iterator it = m_vecPlayerCallbackList.begin();
  while (it != m_vecPlayerCallbackList.end())
  {
    if (*it == pCallback)
      it = m_vecPlayerCallbackList.erase(it);
    else
      it++;
  }
}

/**
* Check for file and print an error if needed
*/
bool XBPython::FileExist(const char* strFile)
{
  if (!strFile)
    return false;

  if (!XFILE::CFile::Exists(strFile))
  {
    CLog::Log(LOGERROR, "Python: Cannot find '%s'", strFile);
    return false;
  }
  return true;
}

void XBPython::RegisterExtensionLib(LibraryLoader *pLib)
{
  if (!pLib)
    return;

  CSingleLock lock(m_critSection);

  CLog::Log(LOGDEBUG,"%s, adding %s (%p)", __FUNCTION__, pLib->GetName(), (void*)pLib);
  m_extensions.push_back(pLib);
}

void XBPython::UnregisterExtensionLib(LibraryLoader *pLib)
{
  if (!pLib)
    return;

  CSingleLock lock(m_critSection);
  CLog::Log(LOGDEBUG,"%s, removing %s (0x%p)", __FUNCTION__, pLib->GetName(), (void *)pLib);
  PythonExtensionLibraries::iterator iter = m_extensions.begin();
  while (iter != m_extensions.end())
  {
    if (*iter == pLib)
    {
      m_extensions.erase(iter);
      break;
    }
    iter++;
  }
}

void XBPython::UnloadExtensionLibs()
{
  CLog::Log(LOGDEBUG,"%s, clearing python extension libraries", __FUNCTION__);
  CSingleLock lock(m_critSection);
  PythonExtensionLibraries::iterator iter = m_extensions.begin();
  while (iter != m_extensions.end())
  {
      DllLoaderContainer::ReleaseModule(*iter);
      iter++;
  }
  m_extensions.clear();
}

void XBPython::InitializeInterpreter()
{
  InitXBMCModule(); // init xbmc modules
  InitPluginModule(); // init xbmcplugin modules
  InitGUIModule(); // init xbmcgui modules
  InitAddonModule(); // init xbmcaddon modules
  InitVFSModule(); // init xbmcvfs modules
  
  // redirecting default output to debug console
  if (PyRun_SimpleString(""
        "import xbmc\n"
        "class xbmcout:\n"
        "\tdef write(self, data):\n"
        "\t\txbmc.output(data)\n"
        "\tdef close(self):\n"
        "\t\txbmc.output('.')\n"
        "\tdef flush(self):\n"
        "\t\txbmc.output('.')\n"
        "\n"
        "import sys\n"
        "sys.stdout = xbmcout()\n"
        "sys.stderr = xbmcout()\n"
        "print '-->Python Interpreter Initialized<--'\n"
        "") == -1)
  {
    CLog::Log(LOGFATAL, "Python Initialize Error");
  }
}

void XBPython::DeInitializeInterpreter()
{
  DeinitXBMCModule();
  DeinitPluginModule();
  DeinitGUIModule();
  DeinitAddonModule();
  DeinitVFSModule();
}

/**
* Should be called before executing a script
*/
void XBPython::Initialize()
{
  CLog::Log(LOGINFO, "initializing python engine. ");
  CSingleLock lock(m_critSection);
  m_iDllScriptCounter++;
  if (!m_bInitialized)
  {
      m_pDll = DllLoaderContainer::LoadModule(PYTHON_DLL, NULL, true);

      if (!m_pDll || !python_load_dll(*m_pDll))
      {
        CLog::Log(LOGFATAL, "Python: error loading python24.dll");
        Finalize();
        return;
      }

      // first we check if all necessary files are installed
#ifndef _LINUX
      if(!FileExist("special://xbmc/system/python/DLLs/_socket.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/_ssl.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/bz2.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/pyexpat.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/select.pyd") ||
#ifndef HAVE_LIBPYTHON2_6
        !FileExist("special://xbmc/system/python/DLLs/zlib.pyd") ||
#endif
        !FileExist("special://xbmc/system/python/DLLs/unicodedata.pyd"))
      {
        CLog::Log(LOGERROR, "Python: Missing files, unable to execute script");
        Finalize();
        return;
      }
#endif


      // Info about interesting python envvars available
      // at http://docs.python.org/using/cmdline.html#environment-variables

#if (!defined USE_EXTERNAL_PYTHON)
#ifdef _LINUX
      // Required for python to find optimized code (pyo) files
      setenv("PYTHONOPTIMIZE", "1", 1);
      setenv("PYTHONHOME", _P("special://xbmc/system/python").c_str(), 1);
#ifdef __APPLE__
      // OSX uses contents from extracted zip, 3X to 4X times faster during Py_Initialize
      setenv("PYTHONPATH", _P("special://xbmc/system/python/Lib").c_str(), 1);
#else
      setenv("PYTHONPATH", _P("special://xbmcbin/system/python/python24.zip").c_str(), 1);
#endif /* __APPLE__ */
      setenv("PYTHONCASEOK", "1", 1);
      CLog::Log(LOGDEBUG, "Python wrapper library linked with internal Python library");
#endif /* _LINUX */
#elif !defined(_WIN32)
      /* PYTHONOPTIMIZE is set off intentionally when using external Python.
         Reason for this is because we cannot be sure what version of Python
         was used to compile the various Python object files (i.e. .pyo,
         .pyc, etc.). */
      #if defined(__APPLE__) && defined(__arm__)
          // using external python, it's build looking for xxx/lib/python2.6
          // so point it to frameworks/usr which is where python2.6 is located
          setenv("PYTHONHOME", _P("special://frameworks/usr").c_str(), 1);
          setenv("PYTHONPATH", _P("special://frameworks/usr").c_str(), 1);
          CLog::Log(LOGDEBUG, "PYTHONHOME -> %s", _P("special://frameworks/usr").c_str());
          CLog::Log(LOGDEBUG, "PYTHONPATH -> %s", _P("special://frameworks/usr").c_str());
      #endif
      setenv("PYTHONCASEOK", "1", 1); //This line should really be removed
      CLog::Log(LOGDEBUG, "Python wrapper library linked with system Python library");
#endif /* USE_EXTERNAL_PYTHON */

      Py_Initialize();
      // If this is not the first time we initialize Python, the interpreter
      // lock already exists and we need to lock it as PyEval_InitThreads
      // would not do that in that case.
#if defined(__APPLE__) && defined(USE_EXTERNAL_PYTHON)
      // grrr, we hang at PyEval_ThreadsInitialized after unloading/loading python
      PyEval_InitThreads();
#else
      if (PyEval_ThreadsInitialized())
        PyEval_AcquireLock();
      else
        PyEval_InitThreads();
#endif
      char* python_argv[1] = { (char*)"" } ;
      PySys_SetArgv(1, python_argv);

      InitXBMCTypes();
      InitGUITypes();
      InitPluginTypes();
      InitAddonTypes();

      if (!(m_mainThreadState = PyThreadState_Get()))
        CLog::Log(LOGERROR, "Python threadstate is NULL.");
      PyEval_ReleaseLock();

      m_bInitialized = true;
  }
}

/**
* Should be called when a script is finished
*/
void XBPython::FinalizeScript()
{
  CSingleLock lock(m_critSection);
  // for linux - we never release the library. its loaded and stays in memory.
  if (m_iDllScriptCounter)
    m_iDllScriptCounter--;
  else
    CLog::Log(LOGERROR, "Python script counter attempted to become negative");
  m_endtime = CTimeUtils::GetTimeMS();
}
void XBPython::Finalize()
{
  if (m_bInitialized)
  {
    CLog::Log(LOGINFO, "Python, unloading python24.dll because no scripts are running anymore");

    PyEval_AcquireLock();
    PyThreadState_Swap(m_mainThreadState);

    Py_Finalize();
    PyEval_ReleaseLock();

    UnloadExtensionLibs();

    // first free all dlls loaded by python, after that python24.dll (this is done by UnloadPythonDlls
    DllLoaderContainer::UnloadPythonDlls();
#ifdef _LINUX
    // we can't release it on windows, as this is done in UnloadPythonDlls() for win32 (see above).
    // The implementation for linux and os x needs looking at - UnloadPythonDlls() currently only searches for "python24.dll"
    DllLoaderContainer::ReleaseModule(m_pDll);
#endif
    m_hModule         = NULL;
    m_mainThreadState = NULL;
    m_bInitialized    = false;
  }
}

void XBPython::FreeResources()
{
  CSingleLock lock(m_critSection);
  if (m_bInitialized)
  {
    // cleanup threads that are still running
    PyList::iterator it = m_vecPyList.begin();
    while (it != m_vecPyList.end())
    {
      lock.Leave(); //unlock here because the python thread might lock when it exits
      delete it->pyThread;
      lock.Enter();
      it = m_vecPyList.erase(it);
      FinalizeScript();
    }
  }
}

void XBPython::Process()
{
  if (m_bLogin)
  {
    m_bLogin = false;

    // autoexec.py - profile
    CStdString strAutoExecPy = _P("special://profile/autoexec.py");

    if ( XFILE::CFile::Exists(strAutoExecPy) )
      evalFile(strAutoExecPy);
    else
      CLog::Log(LOGDEBUG, "%s - no profile autoexec.py (%s) found, skipping", __FUNCTION__, strAutoExecPy.c_str());
  }

  CSingleLock lock(m_critSection);

  if (m_bInitialized)
  {
    PyList::iterator it = m_vecPyList.begin();
    while (it != m_vecPyList.end())
    {
      //delete scripts which are done
      if (it->bDone)
      {
        delete it->pyThread;
        it = m_vecPyList.erase(it);
        FinalizeScript();
      }
      else ++it;
    }

    if(m_iDllScriptCounter == 0 && m_endtime + 10000 < CTimeUtils::GetTimeMS())
      Finalize();
  }
}

bool XBPython::StopScript(const CStdString &path)
{
  int id = getScriptId(path);
  if (id != -1)
  {
    /* if we are here we already know that this script is running.
     * But we will check it again to be sure :)
     */
    if (isRunning(id))
    {
      stopScript(id);
      return true;
    }
  }
  return false;
}

int XBPython::evalFile(const CStdString &src)
{
  std::vector<CStdString> argv;
  return evalFile(src, argv);
}
// execute script, returns -1 if script doesn't exist
int XBPython::evalFile(const CStdString &src, const std::vector<CStdString> &argv)
{
  CSingleExit ex(g_graphicsContext);
  CSingleLock lock(m_critSection);
  // return if file doesn't exist
  if (!XFILE::CFile::Exists(src))
  {
    CLog::Log(LOGERROR, "Python script \"%s\" does not exist", CSpecialProtocol::TranslatePath(src).c_str());
    return -1;
  }

  // check if locked
  if (g_settings.GetCurrentProfile().programsLocked() && !g_passwordManager.IsMasterLockUnlocked(true))
    return -1;

  Initialize();

  if (!m_bInitialized) return -1;

  m_nextid++;
  XBPyThread *pyThread = new XBPyThread(this, m_nextid);
  pyThread->setArgv(argv);
  pyThread->evalFile(src);
  PyElem inf;
  inf.id        = m_nextid;
  inf.bDone     = false;
  inf.strFile   = src;
  inf.pyThread  = pyThread;

  m_vecPyList.push_back(inf);

  return m_nextid;
}

void XBPython::setDone(int id)
{
  CSingleLock lock(m_critSection);
  PyList::iterator it = m_vecPyList.begin();
  while (it != m_vecPyList.end())
  {
    if (it->id == id)
    {
      if (it->pyThread->isStopping())
        CLog::Log(LOGINFO, "Python script interrupted by user");
      else
        CLog::Log(LOGINFO, "Python script stopped");
      it->bDone = true;
    }
    ++it;
  }
}

void XBPython::stopScript(int id)
{
  CSingleExit ex(g_graphicsContext);
  CSingleLock lock(m_critSection);
  PyList::iterator it = m_vecPyList.begin();
  while (it != m_vecPyList.end())
  {
    if (it->id == id) {
      CLog::Log(LOGINFO, "Stopping script with id: %i", id);
      it->pyThread->stop();
      return;
    }
    ++it;
  }
}

PyThreadState *XBPython::getMainThreadState()
{
  CSingleLock lock(m_critSection);
  return m_mainThreadState;
}

int XBPython::ScriptsSize()
{
  CSingleLock lock(m_critSection);
  return m_vecPyList.size();
}

const char* XBPython::getFileName(int scriptId)
{
  const char* cFileName = NULL;

  CSingleLock lock(m_critSection);
  PyList::iterator it = m_vecPyList.begin();
  while (it != m_vecPyList.end())
  {
    if (it->id == scriptId)
      cFileName = it->strFile.c_str();
    ++it;
  }

  return cFileName;
}

int XBPython::getScriptId(const CStdString &strFile)
{
  int iId = -1;

  CSingleLock lock(m_critSection);

  PyList::iterator it = m_vecPyList.begin();
  while (it != m_vecPyList.end())
  {
    if (it->strFile == strFile)
      iId = it->id;
    ++it;
  }

  return iId;
}

bool XBPython::isRunning(int scriptId)
{
  CSingleLock lock(m_critSection);

  for(PyList::iterator it = m_vecPyList.begin(); it != m_vecPyList.end(); it++)
  {
    if (it->id == scriptId)
    {
      if(it->bDone)
        return false;
      else
        return true;
    }
  }
  return false;
}

bool XBPython::isStopping(int scriptId)
{
  bool bStopping = false;

  CSingleLock lock(m_critSection);
  PyList::iterator it = m_vecPyList.begin();
  while (it != m_vecPyList.end())
  {
    if (it->id == scriptId)
      bStopping = it->pyThread->isStopping();
    ++it;
  }

  return bStopping;
}

int XBPython::GetPythonScriptId(int scriptPosition)
{
  CSingleLock lock(m_critSection);
  return (int)m_vecPyList[scriptPosition].id;
}

void XBPython::PulseGlobalEvent()
{
  SetEvent(m_globalEvent);
}

void XBPython::WaitForEvent(HANDLE hEvent, unsigned int timeout)
{
  // wait for either this event our our global event
  HANDLE handles[2] = { hEvent, m_globalEvent };
  WaitForMultipleObjects(2, handles, FALSE, timeout);
  ResetEvent(m_globalEvent);
}

// execute script, returns -1 if script doesn't exist
int XBPython::evalString(const CStdString &src, const std::vector<CStdString> &argv)
{
  CLog::Log(LOGDEBUG, "XBPython::evalString (python)");
  CSingleLock lock(m_critSection);

  Initialize();

  if (!m_bInitialized)
  {
    CLog::Log(LOGERROR, "XBPython::evalString, python not initialized (python)");
    return -1;
  }

  // Previous implementation would create a new thread for every script
  m_nextid++;
  XBPyThread *pyThread = new XBPyThread(this, m_nextid);
  pyThread->setArgv(argv);
  pyThread->evalString(src);

  PyElem inf;
  inf.id        = m_nextid;
  inf.bDone     = false;
  inf.strFile   = "<string>";
  inf.pyThread  = pyThread;

  m_vecPyList.push_back(inf);

  return m_nextid;
}
