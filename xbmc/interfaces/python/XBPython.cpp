/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif

// python.h should always be included first before any other includes
#include <Python.h>

#include <algorithm>

#include "system.h"
#include "cores/DllLoader/DllLoaderContainer.h"
#include "GUIPassword.h"
#include "XBPython.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "profiles/ProfilesManager.h"
#include "utils/log.h"
#include "pythreadstate.h"
#include "utils/TimeUtils.h"
#include "Util.h"
#include "guilib/GraphicContext.h"
#ifdef TARGET_WINDOWS
#include "utils/Environment.h"
#endif

#include "threads/SystemClock.h"
#include "addons/Addon.h"
#include "interfaces/AnnouncementManager.h"

#include "interfaces/legacy/Monitor.h"
#include "interfaces/legacy/AddonUtils.h"
#include "interfaces/python/PythonInvoker.h"

using namespace ANNOUNCEMENT;

namespace PythonBindings {
  void initModule_xbmcgui(void);
  void initModule_xbmc(void);
  void initModule_xbmcplugin(void);
  void initModule_xbmcaddon(void);
  void initModule_xbmcvfs(void);
}

using namespace PythonBindings;

XBPython::XBPython()
{
  m_bInitialized      = false;
  m_mainThreadState   = NULL;
  m_ThreadId          = CThread::GetCurrentThreadId();
  m_iDllScriptCounter = 0;
  m_endtime           = 0;
  m_pDll              = NULL;
  m_vecPlayerCallbackList.clear();
  m_vecMonitorCallbackList.clear();

  CAnnouncementManager::AddAnnouncer(this);
}

XBPython::~XBPython()
{
  TRACE;
  CAnnouncementManager::RemoveAnnouncer(this);
}

#define LOCK_AND_COPY(type, dest, src) \
  if (!m_bInitialized) return; \
  CSingleLock lock(src); \
  src.hadSomethingRemoved = false; \
  type dest; \
  dest = src

#define CHECK_FOR_ENTRY(l,v) \
  (l.hadSomethingRemoved ? (std::find(l.begin(),l.end(),v) != l.end()) : true)

void XBPython::Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (flag & VideoLibrary)
  {
   if (strcmp(message, "OnScanFinished") == 0)
     OnDatabaseUpdated("video");
   else if (strcmp(message, "OnScanStarted") == 0)
     OnDatabaseScanStarted("video");
  }
  else if (flag & AudioLibrary)
  {
   if (strcmp(message, "OnScanFinished") == 0)
     OnDatabaseUpdated("music");
   else if (strcmp(message, "OnScanStarted") == 0)
     OnDatabaseScanStarted("music");
  }
  else if (flag & GUI)
  {
   if (strcmp(message, "OnScreensaverDeactivated") == 0)
     OnScreensaverDeactivated();
   else if (strcmp(message, "OnScreensaverActivated") == 0)
     OnScreensaverActivated();
  }
}

// message all registered callbacks that we started playing
void XBPython::OnPlayBackStarted()
{
  TRACE;
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackStarted();
  }
}

// message all registered callbacks that we paused playing
void XBPython::OnPlayBackPaused()
{
  TRACE;
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackPaused();
  }
}

// message all registered callbacks that we resumed playing
void XBPython::OnPlayBackResumed()
{
  TRACE;
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackResumed();
  }
}

// message all registered callbacks that xbmc stopped playing
void XBPython::OnPlayBackEnded()
{
  TRACE;
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackEnded();
  }
}

// message all registered callbacks that user stopped playing
void XBPython::OnPlayBackStopped()
{
  TRACE;
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackStopped();
  }
}

// message all registered callbacks that playback speed changed (FF/RW)
void XBPython::OnPlayBackSpeedChanged(int iSpeed)
{
  TRACE;
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackSpeedChanged(iSpeed);
  }
}

// message all registered callbacks that player is seeking
void XBPython::OnPlayBackSeek(int iTime, int seekOffset)
{
  TRACE;
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackSeek(iTime, seekOffset);
  }
}

// message all registered callbacks that player chapter seeked
void XBPython::OnPlayBackSeekChapter(int iChapter)
{
  TRACE;
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackSeekChapter(iChapter);
  }
}

// message all registered callbacks that next item has been queued
void XBPython::OnQueueNextItem()
{
  TRACE;
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnQueueNextItem();
  }
}

void XBPython::RegisterPythonPlayerCallBack(IPlayerCallback* pCallback)
{
  TRACE;
  CSingleLock lock(m_vecPlayerCallbackList);
  m_vecPlayerCallbackList.push_back(pCallback);
}

void XBPython::UnregisterPythonPlayerCallBack(IPlayerCallback* pCallback)
{
  TRACE;
  CSingleLock lock(m_vecPlayerCallbackList);
  PlayerCallbackList::iterator it = m_vecPlayerCallbackList.begin();
  while (it != m_vecPlayerCallbackList.end())
  {
    if (*it == pCallback)
    {
      it = m_vecPlayerCallbackList.erase(it);
      m_vecPlayerCallbackList.hadSomethingRemoved = true;
    }
    else
      ++it;
  }
}

void XBPython::RegisterPythonMonitorCallBack(XBMCAddon::xbmc::Monitor* pCallback)
{
  TRACE;
  CSingleLock lock(m_vecMonitorCallbackList);
  m_vecMonitorCallbackList.push_back(pCallback);
}

void XBPython::UnregisterPythonMonitorCallBack(XBMCAddon::xbmc::Monitor* pCallback)
{
  TRACE;
  CSingleLock lock(m_vecMonitorCallbackList);
  MonitorCallbackList::iterator it = m_vecMonitorCallbackList.begin();
  while (it != m_vecMonitorCallbackList.end())
  {
    if (*it == pCallback)
    {
      it = m_vecMonitorCallbackList.erase(it);
      m_vecMonitorCallbackList.hadSomethingRemoved = true;
    }
    else
      ++it;
  }
}

void XBPython::OnSettingsChanged(const CStdString &ID)
{
  TRACE;
  LOCK_AND_COPY(std::vector<XBMCAddon::xbmc::Monitor*>,tmp,m_vecMonitorCallbackList);
  for (MonitorCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecMonitorCallbackList,(*it)) && ((*it)->GetId() == ID))
      (*it)->OnSettingsChanged();
  }
}

void XBPython::OnScreensaverActivated()
{
  TRACE;
  LOCK_AND_COPY(std::vector<XBMCAddon::xbmc::Monitor*>,tmp,m_vecMonitorCallbackList);
  for (MonitorCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecMonitorCallbackList,(*it)))
      (*it)->OnScreensaverActivated();
  }
}

void XBPython::OnScreensaverDeactivated()
{
  TRACE;
  LOCK_AND_COPY(std::vector<XBMCAddon::xbmc::Monitor*>,tmp,m_vecMonitorCallbackList);
  for (MonitorCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecMonitorCallbackList,(*it)))
      (*it)->OnScreensaverDeactivated();
  }
}

void XBPython::OnDatabaseUpdated(const std::string &database)
{
  TRACE;
  LOCK_AND_COPY(std::vector<XBMCAddon::xbmc::Monitor*>,tmp,m_vecMonitorCallbackList);
  for (MonitorCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecMonitorCallbackList,(*it)))
      (*it)->OnDatabaseUpdated(database);
  }
}

void XBPython::OnDatabaseScanStarted(const std::string &database)
{
  TRACE;
  LOCK_AND_COPY(std::vector<XBMCAddon::xbmc::Monitor*>,tmp,m_vecMonitorCallbackList);
  for (MonitorCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecMonitorCallbackList,(*it)))
      (*it)->OnDatabaseScanStarted(database);
  }
}

void XBPython::OnAbortRequested(const CStdString &ID)
{
  TRACE;
  LOCK_AND_COPY(std::vector<XBMCAddon::xbmc::Monitor*>,tmp,m_vecMonitorCallbackList);
  for (MonitorCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecMonitorCallbackList,(*it)))
    {
      if (ID.IsEmpty())
        (*it)->OnAbortRequested();
      else if ((*it)->GetId() == ID)
        (*it)->OnAbortRequested();
    }
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

  CLog::Log(LOGDEBUG, "%s, adding %s (%p)", __FUNCTION__, pLib->GetName(), (void*)pLib);
  m_extensions.push_back(pLib);
}

void XBPython::UnregisterExtensionLib(LibraryLoader *pLib)
{
  if (!pLib)
    return;

  CSingleLock lock(m_critSection);
  CLog::Log(LOGDEBUG, "%s, removing %s (0x%p)", __FUNCTION__, pLib->GetName(), (void *)pLib);
  PythonExtensionLibraries::iterator iter = m_extensions.begin();
  while (iter != m_extensions.end())
  {
    if (*iter == pLib)
    {
      m_extensions.erase(iter);
      break;
    }
    ++iter;
  }
}

void XBPython::UnloadExtensionLibs()
{
  CLog::Log(LOGDEBUG, "%s, clearing python extension libraries", __FUNCTION__);
  CSingleLock lock(m_critSection);
  PythonExtensionLibraries::iterator iter = m_extensions.begin();
  while (iter != m_extensions.end())
  {
      DllLoaderContainer::ReleaseModule(*iter);
      ++iter;
  }
  m_extensions.clear();
}

#define MODULE "xbmc"

#define RUNSCRIPT_PRAMBLE \
        "" \
        "import " MODULE "\n" \
        "xbmc.abortRequested = False\n" \
        "class xbmcout:\n" \
        "\tdef __init__(self, loglevel=" MODULE ".LOGNOTICE):\n" \
        "\t\tself.ll=loglevel\n" \
        "\tdef write(self, data):\n" \
        "\t\t" MODULE ".log(data,self.ll)\n" \
        "\tdef close(self):\n" \
        "\t\t" MODULE ".log('.')\n" \
        "\tdef flush(self):\n" \
        "\t\t" MODULE ".log('.')\n" \
        "import sys\n" \
        "sys.stdout = xbmcout()\n" \
        "sys.stderr = xbmcout(" MODULE ".LOGERROR)\n"

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
  TRACE;
  {
    GilSafeSingleLock lock(m_critSection);
    initModule_xbmcgui();
    initModule_xbmc();
    initModule_xbmcplugin();
    initModule_xbmcaddon();
    initModule_xbmcvfs();
  }

  CStdString addonVer = ADDON::GetXbmcApiVersionDependency(addon);
  bool bwcompatMode = (addon.get() == NULL || (ADDON::AddonVersion(addonVer) <= ADDON::AddonVersion("1.0")));
  const char* runscript = bwcompatMode ? RUNSCRIPT_BWCOMPATIBLE : RUNSCRIPT_COMPLIANT;

  // redirecting default output to debug console
  if (PyRun_SimpleString(runscript) == -1)
    CLog::Log(LOGFATAL, "Python Initialize Error");
}

void XBPython::DeInitializeInterpreter()
{
  TRACE;
}

/**
* Should be called before executing a script
*/
bool XBPython::InitializeEngine()
{
  TRACE;
  CLog::Log(LOGINFO, "initializing python engine.");
  CSingleLock lock(m_critSection);
  m_iDllScriptCounter++;
  if (!m_bInitialized)
  {
      // first we check if all necessary files are installed
#ifndef TARGET_POSIX
      if(!FileExist("special://xbmc/system/python/DLLs/_socket.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/_ssl.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/bz2.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/pyexpat.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/select.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/unicodedata.pyd"))
      {
        CLog::Log(LOGERROR, "Python: Missing files, unable to execute script");
        Finalize();
        return false;
      }
#endif


// Darwin packs .pyo files, we need PYTHONOPTIMIZE on in order to load them.
#if defined(TARGET_DARWIN)
   setenv("PYTHONOPTIMIZE", "1", 1);
#endif
      // Info about interesting python envvars available
      // at http://docs.python.org/using/cmdline.html#environment-variables

#if !defined(TARGET_WINDOWS) && !defined(TARGET_ANDROID)
      /* PYTHONOPTIMIZE is set off intentionally when using external Python.
         Reason for this is because we cannot be sure what version of Python
         was used to compile the various Python object files (i.e. .pyo,
         .pyc, etc.). */
        // check if we are running as real xbmc.app or just binary
      if (!CUtil::GetFrameworksPath(true).IsEmpty())
      {
        // using external python, it's build looking for xxx/lib/python2.6
        // so point it to frameworks which is where python2.6 is located
        setenv("PYTHONHOME", CSpecialProtocol::TranslatePath("special://frameworks").c_str(), 1);
        setenv("PYTHONPATH", CSpecialProtocol::TranslatePath("special://frameworks").c_str(), 1);
        CLog::Log(LOGDEBUG, "PYTHONHOME -> %s", CSpecialProtocol::TranslatePath("special://frameworks").c_str());
        CLog::Log(LOGDEBUG, "PYTHONPATH -> %s", CSpecialProtocol::TranslatePath("special://frameworks").c_str());
      }
      setenv("PYTHONCASEOK", "1", 1); //This line should really be removed
#elif defined(TARGET_WINDOWS)
      // because the third party build of python is compiled with vs2008 we need
      // a hack to set the PYTHONPATH
      CStdString buf;
      buf = "PYTHONPATH=" + CSpecialProtocol::TranslatePath("special://xbmc/system/python/DLLs") + ";" + CSpecialProtocol::TranslatePath("special://xbmc/system/python/Lib");
      CEnvironment::putenv(buf);
      buf = "PYTHONOPTIMIZE=1";
      CEnvironment::putenv(buf);
      buf = "PYTHONHOME=" + CSpecialProtocol::TranslatePath("special://xbmc/system/python");
      CEnvironment::putenv(buf);
      buf = "OS=win32";
      CEnvironment::putenv(buf);

#elif defined(TARGET_ANDROID)
      CStdString apkPath = getenv("XBMC_ANDROID_APK");
      apkPath += "/assets/python2.6";
      setenv("PYTHONHOME",apkPath.c_str(), 1);
      setenv("PYTHONPATH", "", 1);
      setenv("PYTHONOPTIMIZE","",1);
      setenv("PYTHONNOUSERSITE","1",1);
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

      if (!(m_mainThreadState = PyThreadState_Get()))
        CLog::Log(LOGERROR, "Python threadstate is NULL.");
      PyEval_ReleaseLock();

      m_bInitialized = true;
  }

  return m_bInitialized;
}

/**
* Should be called when a script is finished
*/
void XBPython::FinalizeScript()
{
  TRACE;
  CSingleLock lock(m_critSection);
  // for linux - we never release the library. its loaded and stays in memory.
  if (m_iDllScriptCounter)
    m_iDllScriptCounter--;
  else
    CLog::Log(LOGERROR, "Python script counter attempted to become negative");
  m_endtime = XbmcThreads::SystemClockMillis();
}

// Always called with the lock held on m_critSection
void XBPython::Finalize()
{
  TRACE;
  if (m_bInitialized)
  {
    CLog::Log(LOGINFO, "Python, unloading python shared library because no scripts are running anymore");

    // set the m_bInitialized flag before releasing the lock. This will prevent
    // Other methods that rely on this flag from an incorrect interpretation.
    m_bInitialized    = false;
    PyThreadState* curTs = (PyThreadState*)m_mainThreadState;
    m_mainThreadState = NULL; // clear the main thread state before releasing the lock
    {
      CSingleExit exit(m_critSection);
      PyEval_AcquireLock();
      PyThreadState_Swap(curTs);

      Py_Finalize();
      PyEval_ReleaseLock();
    }

#if !(defined(TARGET_DARWIN) || defined(TARGET_WINDOWS))
    UnloadExtensionLibs();
#endif

    // first free all dlls loaded by python, after that python24.dll (this is done by UnloadPythonDlls
#if !(defined(TARGET_DARWIN) || defined(TARGET_WINDOWS))
    DllLoaderContainer::UnloadPythonDlls();
#endif
#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN) && !defined(TARGET_FREEBSD)
    // we can't release it on windows, as this is done in UnloadPythonDlls() for win32 (see above).
    // The implementation for linux needs looking at - UnloadPythonDlls() currently only searches for "python24.dll"
    // The implementation for osx can never unload the python dylib.
    DllLoaderContainer::ReleaseModule(m_pDll);
#endif
  }
}

void XBPython::Uninitialize()
{
  LOCK_AND_COPY(std::vector<PyElem>,tmpvec,m_vecPyList);
  m_vecPyList.clear();
  m_vecPyList.hadSomethingRemoved = true;

  lock.Leave(); //unlock here because the python thread might lock when it exits

  // cleanup threads that are still running
  tmpvec.clear(); // boost releases the XBPyThreads which, if deleted, calls FinalizeScript
}

void XBPython::Process()
{
  if (m_bInitialized)
  {
    PyList tmpvec;
    CSingleLock lock(m_vecPyList);
    for (PyList::iterator it = m_vecPyList.begin(); it != m_vecPyList.end();)
    {
      if (it->bDone)
      {
        tmpvec.push_back(*it);
        it = m_vecPyList.erase(it);
        m_vecPyList.hadSomethingRemoved = true;
      }
      else
        ++it;
    }
    lock.Leave();

    //delete scripts which are done
    tmpvec.clear(); // boost releases the XBPyThreads which, if deleted, calls FinalizeScript

    CSingleLock l2(m_critSection);
    if(m_iDllScriptCounter == 0 && (XbmcThreads::SystemClockMillis() - m_endtime) > 10000 )
    {
      Finalize();
    }
  }
}

void XBPython::OnScriptStarted(ILanguageInvoker *invoker)
{
  if (invoker == NULL)
    return;

  if (!m_bInitialized)
    return;

  PyElem inf;
  inf.id        = invoker->GetId();
  inf.bDone     = false;
  inf.pyThread  = static_cast<CPythonInvoker*>(invoker);
  CSingleLock lock(m_vecPyList);
  m_vecPyList.push_back(inf);
}

void XBPython::OnScriptEnded(ILanguageInvoker *invoker)
{
  CSingleLock lock(m_vecPyList);
  PyList::iterator it = m_vecPyList.begin();
  while (it != m_vecPyList.end())
  {
    if (it->id == invoker->GetId())
    {
      if (it->pyThread->IsStopping())
        CLog::Log(LOGINFO, "Python script interrupted by user");
      else
        CLog::Log(LOGINFO, "Python script stopped");
      it->bDone = true;
    }
    ++it;
  }
}

ILanguageInvoker* XBPython::CreateInvoker()
{
  return new CPythonInvoker(this);
}

void XBPython::PulseGlobalEvent()
{
  m_globalEvent.Set();
}

bool XBPython::WaitForEvent(CEvent& hEvent, unsigned int milliseconds)
{
  // wait for either this event our our global event
  XbmcThreads::CEventGroup eventGroup(&hEvent, &m_globalEvent, NULL);
  CEvent* ret = eventGroup.wait(milliseconds);
  if (ret)
    m_globalEvent.Reset();
  return ret != NULL;
}
