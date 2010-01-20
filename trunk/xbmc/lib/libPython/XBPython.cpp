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
  #include "Python/Include/Python.h"
#endif
#include "cores/DllLoader/DllLoaderContainer.h"
#include "GUIPassword.h"

#include "XBPython.h"
#include "XBPythonDll.h"
#include "Settings.h"
#include "Profile.h"
#include "FileSystem/File.h"
#include "FileSystem/SpecialProtocol.h"
#include "utils/log.h"
#include "utils/SingleLock.h"

XBPython g_pythonParser;

#ifndef _LINUX
#define PYTHON_DLL "special://xbmc/system/python/python24.dll"
#else
#if defined(__APPLE__)
#if defined(__POWERPC__)
#define PYTHON_DLL "special://xbmc/system/python/python24-powerpc-osx.so"
#else
#define PYTHON_DLL "special://xbmc/system/python/python24-x86-osx.so"
#endif
#elif defined(__x86_64__)
#if (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmc/system/python/python26-x86_64-linux.so"
#elif (defined HAVE_LIBPYTHON2_5)
#define PYTHON_DLL "special://xbmc/system/python/python25-x86_64-linux.so"
#else
#define PYTHON_DLL "special://xbmc/system/python/python24-x86_64-linux.so"
#endif
#elif defined(_POWERPC)
#if (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmc/system/python/python26-powerpc-linux.so"
#elif (defined HAVE_LIBPYTHON2_5)
#define PYTHON_DLL "special://xbmc/system/python/python25-powerpc-linux.so"
#else
#define PYTHON_DLL "special://xbmc/system/python/python24-powerpc-linux.so"
#endif
#elif defined(_POWERPC64)
#if (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmc/system/python/python26-powerpc64-linux.so"
#elif (defined HAVE_LIBPYTHON2_5)
#define PYTHON_DLL "special://xbmc/system/python/python25-powerpc64-linux.so"
#else
#define PYTHON_DLL "special://xbmc/system/python/python24-powerpc64-linux.so"
#endif
#else /* !__x86_64__ && !__powerpc__ */
#if (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmc/system/python/python26-i486-linux.so"
#elif (defined HAVE_LIBPYTHON2_5)
#define PYTHON_DLL "special://xbmc/system/python/python25-i486-linux.so"
#else
#define PYTHON_DLL "special://xbmc/system/python/python24-i486-linux.so"
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
}

XBPython::XBPython()
{
  m_bInitialized      = false;
  m_bStartup          = false;
  m_bLogin            = false;
  m_nextid            = 0;
  m_mainThreadState   = NULL;
  m_hEvent            = CreateEvent(NULL, false, false, (char*)"pythonEvent");
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

// message all registered callbacks that user stopped playing
void XBPython::OnPlayBackStopped()
{
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
  m_vecPlayerCallbackList.push_back(pCallback);
}

void XBPython::UnregisterPythonPlayerCallBack(IPlayerCallback* pCallback)
{
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
  InitPluginModule(); // init plugin modules
  InitGUIModule(); // init xbmcgui modules

  // redirecting default output to debug console
  if (PyRun_SimpleString(""
        "import xbmc\n"
        "class xbmcout:\n"
        "	def write(self, data):\n"
        "		xbmc.output(data)\n"
        "	def close(self):\n"
        "		xbmc.output('.')\n"
        "	def flush(self):\n"
        "		xbmc.output('.')\n"
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
    if (CThread::IsCurrentThread(m_ThreadId))
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
      if (!FileExist("special://xbmc/system/python/python24.zlib") ||
        !FileExist("special://xbmc/system/python/DLLs/_socket.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/_ssl.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/bz2.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/pyexpat.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/select.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/unicodedata.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/zlib.pyd"))
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
      setenv("PYTHONPATH", _P("special://xbmc/system/python/python24.zip").c_str(), 1);
#endif /* __APPLE__ */
      setenv("PYTHONCASEOK", "1", 1);
      CLog::Log(LOGDEBUG, "Python wrapper library linked with internal Python library");
#endif /* _LINUX */
#else
      /* PYTHONOPTIMIZE is set off intentionally when using external Python.
         Reason for this is because we cannot be sure what version of Python
         was used to compile the various Python object files (i.e. .pyo,
         .pyc, etc.). */
      setenv("PYTHONCASEOK", "1", 1); //This line should really be removed
      CLog::Log(LOGDEBUG, "Python wrapper library linked with system Python library");
#endif /* USE_EXTERNAL_PYTHON */

      Py_Initialize();
      PyEval_InitThreads();

      char* python_argv[1] = { (char*)"" } ;
      PySys_SetArgv(1, python_argv);

      InitXBMCTypes();
      InitGUITypes();
      InitPluginTypes();
      
      if (!(m_mainThreadState = PyThreadState_Get()))
        CLog::Log(LOGERROR, "Python threadstate is NULL.");
      PyEval_ReleaseLock();

      m_bInitialized = true;
      PulseEvent(m_hEvent);
    }
    else
    {
      // only the main thread should initialize python.
      m_iDllScriptCounter--;

      lock.Leave();
      WaitForSingleObject(m_hEvent, INFINITE);
      lock.Enter();
    }
  }
}

/**
* Should be called when a script is finished
*/
void XBPython::Finalize()
{
  CSingleLock lock(m_critSection);
  // for linux - we never release the library. its loaded and stays in memory.
  if (m_iDllScriptCounter)
    m_iDllScriptCounter--;
  else
    CLog::Log(LOGERROR, "Python script counter attempted to become negative");
  if (m_iDllScriptCounter == 0 && m_bInitialized)
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
      Finalize();
    }
  }

  if (m_hEvent)
    CloseHandle(m_hEvent);
}

void XBPython::Process()
{
  if (m_bStartup)
  {
    m_bStartup = false;

    // autoexec.py - userdata
    CStdString strAutoExecPy = _P("special://home/scripts/autoexec.py");

    if ( XFILE::CFile::Exists(strAutoExecPy) )
      evalFile(strAutoExecPy);
    else
      CLog::Log(LOGDEBUG, "%s - no user autoexec.py (%s) found, skipping", __FUNCTION__, strAutoExecPy.c_str());

    // autoexec.py - system
    CStdString strAutoExecPy2 = _P("special://xbmc/scripts/autoexec.py");

    // Make sure special://xbmc & special://home don't point to the same location
    if (strAutoExecPy != strAutoExecPy2)
    {
      if ( XFILE::CFile::Exists(strAutoExecPy2) )
        evalFile(strAutoExecPy2);
      else
        CLog::Log(LOGDEBUG, "%s - no system autoexec.py (%s) found, skipping", __FUNCTION__, strAutoExecPy2.c_str());
    }
  }

  if (m_bLogin)
  {
    m_bLogin = false;

    // autoexec.py - profile
    CStdString strAutoExecPy = _P("special://profile/scripts/autoexec.py");

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
        Finalize();
      }
      else ++it;
    }
  }
}

int XBPython::evalFile(const char *src) { return evalFile(src, 0, NULL); }
// execute script, returns -1 if script doesn't exist
int XBPython::evalFile(const char *src, const unsigned int argc, const char ** argv)
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
  int profile = g_settings.m_iLastLoadedProfileIndex;
  if (profile < (int)g_settings.m_vecProfiles.size() &&
      g_settings.m_vecProfiles[profile].programsLocked() &&
      !g_passwordManager.IsMasterLockUnlocked(true))
  {
      return -1;
  }

  Initialize();

  if (!m_bInitialized) return -1;

  m_nextid++;
  XBPyThread *pyThread = new XBPyThread(this, m_nextid);
  if (argv != NULL)
    pyThread->setArgv(argc, argv);
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

int XBPython::getScriptId(const char* strFile)
{
  int iId = -1;
  
  CSingleLock lock(m_critSection);

  PyList::iterator it = m_vecPyList.begin();
  while (it != m_vecPyList.end())
  {
    if (!stricmp(it->strFile.c_str(), strFile))
      iId = it->id;
    ++it;
  }
  
  return iId;
}

bool XBPython::isRunning(int scriptId)
{
  bool bRunning = false;
  CSingleLock lock(m_critSection); 

  PyList::iterator it = m_vecPyList.begin();
  while (it != m_vecPyList.end())
  {
    if (it->id == scriptId)
      bRunning = true;
    ++it;
  }
  
  return bRunning;
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
int XBPython::evalString(const char *src, const unsigned int argc, const char ** argv)
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
  if (argv != NULL)
    pyThread->setArgv(argc, argv);
  pyThread->evalString(src);
  
  PyElem inf;
  inf.id        = m_nextid;
  inf.bDone     = false;
  inf.strFile   = "<string>";
  inf.pyThread  = pyThread;

  m_vecPyList.push_back(inf);

  return m_nextid;
}
