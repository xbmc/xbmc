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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif

// python.h should always be included first before any other includes
#include <Python.h>

#include "system.h"
#include "cores/DllLoader/DllLoaderContainer.h"
#include "GUIPassword.h"

#include "XBPython.h"
#include "settings/Settings.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "utils/TimeUtils.h"
#include "Util.h"

#include "threads/SystemClock.h"
#include "addons/Addon.h"

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
  m_ThreadId          = CThread::GetCurrentThreadId();
  m_iDllScriptCounter = 0;
  m_vecPlayerCallbackList.clear();
}

XBPython::~XBPython()
{
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

#define RUNSCRIPT_PRAMBLE \
        "" \
        "import xbmc\n" \
        "class xbmcout:\n" \
        "\tdef __init__(self, loglevel=xbmc.LOGNOTICE):\n" \
        "\t\tself.ll=loglevel\n" \
        "\tdef write(self, data):\n" \
        "\t\txbmc.log(data,self.ll)\n" \
        "\tdef close(self):\n" \
        "\t\txbmc.log('.')\n" \
        "\tdef flush(self):\n" \
        "\t\txbmc.log('.')\n" \
        "import sys\n" \
        "sys.stdout = xbmcout()\n" \
        "sys.stderr = xbmcout(xbmc.LOGERROR)\n"

#define RUNSCRIPT_OVERRIDE_HACK \
        "" \
        "import os\n" \
        "def getcwd_xbmc():\n" \
        "  import __main__\n" \
        "  import warnings\n" \
        "  if hasattr(__main__, \"__file__\"):\n" \
        "    warnings.warn(\"os.getcwd() currently lies to you so please use addon.getAddonInfo('path') to find the script's root directory and DO NOT make relative path accesses based on the results of 'os.getcwd.' \", DeprecationWarning, stacklevel=2)\n" \
        "    return os.path.dirname(__main__.__file__)\n" \
        "  else:\n" \
        "    return os.getcwd_original()\n" \
        "" \
        "def chdir_xbmc(dir):\n" \
        "  raise RuntimeError(\"os.chdir not supported in xbmc\")\n" \
        "" \
        "os_getcwd_original = os.getcwd\n" \
        "os.getcwd          = getcwd_xbmc\n" \
        "os.chdir_orignal   = os.chdir\n" \
        "os.chdir           = chdir_xbmc\n" \
        ""
 
#define RUNSCRIPT_POSTSCRIPT \
        "print '-->Python Interpreter Initialized<--'\n" \
        ""

#define RUNSCRIPT_BWCOMPATIBLE \
  RUNSCRIPT_PRAMBLE RUNSCRIPT_OVERRIDE_HACK RUNSCRIPT_POSTSCRIPT

#define RUNSCRIPT_COMPLIANT \
  RUNSCRIPT_PRAMBLE RUNSCRIPT_POSTSCRIPT

void XBPython::InitializeInterpreter(ADDON::AddonPtr addon)
{
  InitXBMCModule(); // init xbmc modules
  InitPluginModule(); // init xbmcplugin modules
  InitGUIModule(); // init xbmcgui modules
  InitAddonModule(); // init xbmcaddon modules
  InitVFSModule(); // init xbmcvfs modules

  CStdString addonVer = ADDON::GetXbmcApiVersionDependency(addon);
  bool bwcompatMode = (addon.get() == NULL || (ADDON::AddonVersion(addonVer) <= ADDON::AddonVersion("1.0")));
  const char* runscript = bwcompatMode ? RUNSCRIPT_BWCOMPATIBLE : RUNSCRIPT_COMPLIANT;

  // redirecting default output to debug console
  if (PyRun_SimpleString(runscript) == -1)
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
      // first we check if all necessary files are installed
#ifndef _LINUX
      if(!FileExist("special://xbmc/system/python/DLLs/_socket.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/_ssl.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/bz2.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/pyexpat.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/select.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/unicodedata.pyd"))
      {
        CLog::Log(LOGERROR, "Python: Missing files, unable to execute script");
        Finalize();
        return;
      }
#endif


      // Info about interesting python envvars available
      // at http://docs.python.org/using/cmdline.html#environment-variables

#if !defined(_WIN32)
      /* PYTHONOPTIMIZE is set off intentionally when using external Python.
         Reason for this is because we cannot be sure what version of Python
         was used to compile the various Python object files (i.e. .pyo,
         .pyc, etc.). */
        // check if we are running as real xbmc.app or just binary
      if (!CUtil::GetFrameworksPath(true).IsEmpty())
      {
        // using external python, it's build looking for xxx/lib/python2.6
        // so point it to frameworks which is where python2.6 is located
        setenv("PYTHONHOME", _P("special://frameworks").c_str(), 1);
        setenv("PYTHONPATH", _P("special://frameworks").c_str(), 1);
        CLog::Log(LOGDEBUG, "PYTHONHOME -> %s", _P("special://frameworks").c_str());
        CLog::Log(LOGDEBUG, "PYTHONPATH -> %s", _P("special://frameworks").c_str());
      }
      setenv("PYTHONCASEOK", "1", 1); //This line should really be removed
#elif defined(_WIN32)
      // because the third party build of python is compiled with vs2008 we need
      // a hack to set the PYTHONPATH
      // buf is corrupted after putenv and might need a strdup but it seems to
      // work this way
      CStdString buf;
      buf = "PYTHONPATH=" + _P("special://xbmc/system/python/DLLs") + ";" + _P("special://xbmc/system/python/Lib");
      pgwin32_putenv(buf.c_str());
      buf = "PYTHONOPTIMIZE=1";
      pgwin32_putenv(buf.c_str());
      buf = "PYTHONHOME=" + _P("special://xbmc/system/python");
      pgwin32_putenv(buf.c_str());
      buf = "OS=win32";
      pgwin32_putenv(buf.c_str());

#endif

      if (PyEval_ThreadsInitialized())
        PyEval_AcquireLock();
      else
        PyEval_InitThreads();

      Py_Initialize();
      PyEval_ReleaseLock();

      // If this is not the first time we initialize Python, the interpreter
      // lock already exists and we need to lock it as PyEval_InitThreads
      // would not do that in that case.
      PyEval_AcquireLock();
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
  m_endtime = XbmcThreads::SystemClockMillis();
}
void XBPython::Finalize()
{
  if (m_bInitialized)
  {
    CLog::Log(LOGINFO, "Python, unloading python shared library because no scripts are running anymore");

    PyEval_AcquireLock();
    PyThreadState_Swap((PyThreadState*)m_mainThreadState);

    Py_Finalize();
    PyEval_ReleaseLock();

#if !(defined(__APPLE__) || defined(_WIN32))
    UnloadExtensionLibs();
#endif

    // first free all dlls loaded by python, after that python24.dll (this is done by UnloadPythonDlls
#if !(defined(__APPLE__) || defined(_WIN32))
    DllLoaderContainer::UnloadPythonDlls();
#endif
#if defined(_LINUX) && !defined(__APPLE__)
    // we can't release it on windows, as this is done in UnloadPythonDlls() for win32 (see above).
    // The implementation for linux needs looking at - UnloadPythonDlls() currently only searches for "python24.dll"
    // The implementation for osx can never unload the python dylib.
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
      evalFile(strAutoExecPy,ADDON::AddonPtr());
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

    if(m_iDllScriptCounter == 0 && (XbmcThreads::SystemClockMillis() - m_endtime) > 10000 )
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

int XBPython::evalFile(const CStdString &src, ADDON::AddonPtr addon)
{
  std::vector<CStdString> argv;
  return evalFile(src, argv, addon);
}
// execute script, returns -1 if script doesn't exist
int XBPython::evalFile(const CStdString &src, const std::vector<CStdString> &argv, ADDON::AddonPtr addon)
{
  CSingleExit ex(g_graphicsContext);
  // return if file doesn't exist
  if (!XFILE::CFile::Exists(src))
  {
    CLog::Log(LOGERROR, "Python script \"%s\" does not exist", CSpecialProtocol::TranslatePath(src).c_str());
    return -1;
  }

  // check if locked
  if (g_settings.GetCurrentProfile().programsLocked() && !g_passwordManager.IsMasterLockUnlocked(true))
    return -1;

  CSingleLock lock(m_critSection);
  Initialize();

  if (!m_bInitialized) return -1;

  m_nextid++;
  XBPyThread *pyThread = new XBPyThread(this, m_nextid);
  pyThread->setArgv(argv);
  pyThread->setAddon(addon);
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

void* XBPython::getMainThreadState()
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
  m_globalEvent.Set();
}

void XBPython::WaitForEvent(CEvent& hEvent, unsigned int timeout)
{
  // wait for either this event our our global event
  XbmcThreads::CEventGroup eventGroup(&hEvent, &m_globalEvent, NULL);
  eventGroup.wait(timeout);
  m_globalEvent.Reset();
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
