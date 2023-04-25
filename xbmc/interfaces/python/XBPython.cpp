/*
 *  Copyright (C) 2005-2018 Team Kodi
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

#include "XBPython.h"

#include "ServiceBroker.h"
#include "Util.h"
#include "filesystem/SpecialProtocol.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/legacy/AddonUtils.h"
#include "interfaces/legacy/Monitor.h"
#include "interfaces/python/AddonPythonInvoker.h"
#include "interfaces/python/PythonInvoker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/CharsetConverter.h"
#include "utils/JSONVariantWriter.h"
#include "utils/Variant.h"
#include "utils/log.h"

#ifdef TARGET_WINDOWS
#include "platform/Environment.h"
#endif

#ifdef HAS_WEB_INTERFACE
#include "network/httprequesthandler/python/HTTPPythonWsgiInvoker.h"
#endif

#include <algorithm>

// Only required for Py3 < 3.7
PyThreadState* savestate;

bool XBPython::m_bInitialized = false;

XBPython::XBPython()
{
  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this);
}

XBPython::~XBPython()
{
  XBMC_TRACE;
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);

#if PY_VERSION_HEX >= 0x03070000
  if (Py_IsInitialized())
  {
    PyThreadState_Swap(PyInterpreterState_ThreadHead(PyInterpreterState_Main()));
    Py_Finalize();
  }
#endif
}

#define LOCK_AND_COPY(type, dest, src) \
  if (!m_bInitialized) \
    return; \
  std::unique_lock<CCriticalSection> lock(src); \
  src.hadSomethingRemoved = false; \
  type dest; \
  dest = src

#define CHECK_FOR_ENTRY(l, v) \
  (l.hadSomethingRemoved ? (std::find(l.begin(), l.end(), v) != l.end()) : true)

void XBPython::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                        const std::string& sender,
                        const std::string& message,
                        const CVariant& data)
{
  if (flag & ANNOUNCEMENT::VideoLibrary)
  {
    if (message == "OnScanFinished")
      OnScanFinished("video");
    else if (message == "OnScanStarted")
      OnScanStarted("video");
    else if (message == "OnCleanStarted")
      OnCleanStarted("video");
    else if (message == "OnCleanFinished")
      OnCleanFinished("video");
  }
  else if (flag & ANNOUNCEMENT::AudioLibrary)
  {
    if (message == "OnScanFinished")
      OnScanFinished("music");
    else if (message == "OnScanStarted")
      OnScanStarted("music");
    else if (message == "OnCleanStarted")
      OnCleanStarted("music");
    else if (message == "OnCleanFinished")
      OnCleanFinished("music");
  }
  else if (flag & ANNOUNCEMENT::GUI)
  {
    if (message == "OnScreensaverDeactivated")
      OnScreensaverDeactivated();
    else if (message == "OnScreensaverActivated")
      OnScreensaverActivated();
    else if (message == "OnDPMSDeactivated")
      OnDPMSDeactivated();
    else if (message == "OnDPMSActivated")
      OnDPMSActivated();
  }

  std::string jsonData;
  if (CJSONVariantWriter::Write(
          data, jsonData,
          CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_jsonOutputCompact))
    OnNotification(sender,
                   std::string(ANNOUNCEMENT::AnnouncementFlagToString(flag)) + "." +
                       std::string(message),
                   jsonData);
}

// message all registered callbacks that we started playing
void XBPython::OnPlayBackStarted(const CFileItem& file)
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<void*>, tmp, m_vecPlayerCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList, it))
      ((IPlayerCallback*)it)->OnPlayBackStarted(file);
  }
}

// message all registered callbacks that we changed stream
void XBPython::OnAVStarted(const CFileItem& file)
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<void*>, tmp, m_vecPlayerCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList, it))
      ((IPlayerCallback*)it)->OnAVStarted(file);
  }
}

// message all registered callbacks that we changed stream
void XBPython::OnAVChange()
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<void*>, tmp, m_vecPlayerCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList, it))
      ((IPlayerCallback*)it)->OnAVChange();
  }
}

// message all registered callbacks that we paused playing
void XBPython::OnPlayBackPaused()
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<void*>, tmp, m_vecPlayerCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList, it))
      ((IPlayerCallback*)it)->OnPlayBackPaused();
  }
}

// message all registered callbacks that we resumed playing
void XBPython::OnPlayBackResumed()
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<void*>, tmp, m_vecPlayerCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList, it))
      ((IPlayerCallback*)it)->OnPlayBackResumed();
  }
}

// message all registered callbacks that xbmc stopped playing
void XBPython::OnPlayBackEnded()
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<void*>, tmp, m_vecPlayerCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList, it))
      ((IPlayerCallback*)it)->OnPlayBackEnded();
  }
}

// message all registered callbacks that user stopped playing
void XBPython::OnPlayBackStopped()
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<void*>, tmp, m_vecPlayerCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList, it))
      ((IPlayerCallback*)it)->OnPlayBackStopped();
  }
}

// message all registered callbacks that playback stopped due to error
void XBPython::OnPlayBackError()
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<void*>, tmp, m_vecPlayerCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList, it))
      ((IPlayerCallback*)it)->OnPlayBackError();
  }
}

// message all registered callbacks that playback speed changed (FF/RW)
void XBPython::OnPlayBackSpeedChanged(int iSpeed)
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<void*>, tmp, m_vecPlayerCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList, it))
      ((IPlayerCallback*)it)->OnPlayBackSpeedChanged(iSpeed);
  }
}

// message all registered callbacks that player is seeking
void XBPython::OnPlayBackSeek(int64_t iTime, int64_t seekOffset)
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<void*>, tmp, m_vecPlayerCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList, it))
      ((IPlayerCallback*)it)->OnPlayBackSeek(iTime, seekOffset);
  }
}

// message all registered callbacks that player chapter seeked
void XBPython::OnPlayBackSeekChapter(int iChapter)
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<void*>, tmp, m_vecPlayerCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList, it))
      ((IPlayerCallback*)it)->OnPlayBackSeekChapter(iChapter);
  }
}

// message all registered callbacks that next item has been queued
void XBPython::OnQueueNextItem()
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<void*>, tmp, m_vecPlayerCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList, it))
      ((IPlayerCallback*)it)->OnQueueNextItem();
  }
}

void XBPython::RegisterPythonPlayerCallBack(IPlayerCallback* pCallback)
{
  XBMC_TRACE;
  std::unique_lock<CCriticalSection> lock(m_vecPlayerCallbackList);
  m_vecPlayerCallbackList.push_back(pCallback);
}

void XBPython::UnregisterPythonPlayerCallBack(IPlayerCallback* pCallback)
{
  XBMC_TRACE;
  std::unique_lock<CCriticalSection> lock(m_vecPlayerCallbackList);
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
  XBMC_TRACE;
  std::unique_lock<CCriticalSection> lock(m_vecMonitorCallbackList);
  m_vecMonitorCallbackList.push_back(pCallback);
}

void XBPython::UnregisterPythonMonitorCallBack(XBMCAddon::xbmc::Monitor* pCallback)
{
  XBMC_TRACE;
  std::unique_lock<CCriticalSection> lock(m_vecMonitorCallbackList);
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

void XBPython::OnSettingsChanged(const std::string& ID)
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<XBMCAddon::xbmc::Monitor*>, tmp, m_vecMonitorCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecMonitorCallbackList, it) && (it->GetId() == ID))
      it->OnSettingsChanged();
  }
}

void XBPython::OnScreensaverActivated()
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<XBMCAddon::xbmc::Monitor*>, tmp, m_vecMonitorCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecMonitorCallbackList, it))
      it->OnScreensaverActivated();
  }
}

void XBPython::OnScreensaverDeactivated()
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<XBMCAddon::xbmc::Monitor*>, tmp, m_vecMonitorCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecMonitorCallbackList, it))
      it->OnScreensaverDeactivated();
  }
}

void XBPython::OnDPMSActivated()
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<XBMCAddon::xbmc::Monitor*>, tmp, m_vecMonitorCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecMonitorCallbackList, it))
      it->OnDPMSActivated();
  }
}

void XBPython::OnDPMSDeactivated()
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<XBMCAddon::xbmc::Monitor*>, tmp, m_vecMonitorCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecMonitorCallbackList, it))
      it->OnDPMSDeactivated();
  }
}

void XBPython::OnScanStarted(const std::string& library)
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<XBMCAddon::xbmc::Monitor*>, tmp, m_vecMonitorCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecMonitorCallbackList, it))
      it->OnScanStarted(library);
  }
}

void XBPython::OnScanFinished(const std::string& library)
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<XBMCAddon::xbmc::Monitor*>, tmp, m_vecMonitorCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecMonitorCallbackList, it))
      it->OnScanFinished(library);
  }
}

void XBPython::OnCleanStarted(const std::string& library)
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<XBMCAddon::xbmc::Monitor*>, tmp, m_vecMonitorCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecMonitorCallbackList, it))
      it->OnCleanStarted(library);
  }
}

void XBPython::OnCleanFinished(const std::string& library)
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<XBMCAddon::xbmc::Monitor*>, tmp, m_vecMonitorCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecMonitorCallbackList, it))
      it->OnCleanFinished(library);
  }
}

void XBPython::OnNotification(const std::string& sender,
                              const std::string& method,
                              const std::string& data)
{
  XBMC_TRACE;
  LOCK_AND_COPY(std::vector<XBMCAddon::xbmc::Monitor*>, tmp, m_vecMonitorCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecMonitorCallbackList, it))
      it->OnNotification(sender, method, data);
  }
}

void XBPython::Uninitialize()
{
  // don't handle any more announcements as most scripts are probably already
  // stopped and executing a callback on one of their already destroyed classes
  // would lead to a crash
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);

  LOCK_AND_COPY(std::vector<PyElem>, tmpvec, m_vecPyList);
  m_vecPyList.clear();
  m_vecPyList.hadSomethingRemoved = true;

  lock.unlock(); //unlock here because the python thread might lock when it exits

  // cleanup threads that are still running
  tmpvec.clear();
}

void XBPython::Process()
{
  if (m_bInitialized)
  {
    PyList tmpvec;
    std::unique_lock<CCriticalSection> lock(m_vecPyList);
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
    lock.unlock();

    //delete scripts which are done
    tmpvec.clear();
  }
}

bool XBPython::OnScriptInitialized(ILanguageInvoker* invoker)
{
  if (invoker == NULL)
    return false;

  XBMC_TRACE;
  CLog::Log(LOGDEBUG, "initializing python engine.");
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_iDllScriptCounter++;
  if (!m_bInitialized)
  {
    // Darwin packs .pyo files, we need PYTHONOPTIMIZE on in order to load them.
    // linux built with unified builds only packages the pyo files so need it
#if defined(TARGET_DARWIN) || defined(TARGET_LINUX)
    setenv("PYTHONOPTIMIZE", "1", 1);
#endif
    // Info about interesting python envvars available
    // at http://docs.python.org/using/cmdline.html#environment-variables

#if !defined(TARGET_WINDOWS) && !defined(TARGET_ANDROID)
    // check if we are running as real xbmc.app or just binary
    if (!CUtil::GetFrameworksPath(true).empty())
    {
      // using external python, it's build looking for xxx/lib/python3.8
      // so point it to frameworks which is where python3.8 is located
      setenv("PYTHONHOME", CSpecialProtocol::TranslatePath("special://frameworks").c_str(), 1);
      setenv("PYTHONPATH", CSpecialProtocol::TranslatePath("special://frameworks").c_str(), 1);
      CLog::Log(LOGDEBUG, "PYTHONHOME -> {}",
                CSpecialProtocol::TranslatePath("special://frameworks"));
      CLog::Log(LOGDEBUG, "PYTHONPATH -> {}",
                CSpecialProtocol::TranslatePath("special://frameworks"));
    }
#elif defined(TARGET_WINDOWS)

#ifdef TARGET_WINDOWS_STORE
#ifdef _DEBUG
    CEnvironment::putenv("PYTHONCASEOK=1");
#endif
    CEnvironment::putenv("OS=win10");
#else // TARGET_WINDOWS_DESKTOP
    CEnvironment::putenv("OS=win32");
#endif

    std::wstring pythonHomeW;
    CCharsetConverter::utf8ToW(CSpecialProtocol::TranslatePath("special://xbmc/system/python"),
                           pythonHomeW);
    Py_SetPythonHome(pythonHomeW.c_str());

    std::string pythonPath = CSpecialProtocol::TranslatePath("special://xbmc/system/python/DLLs");
    pythonPath += ";";
    pythonPath += CSpecialProtocol::TranslatePath("special://xbmc/system/python/Lib");
    pythonPath += ";";
    pythonPath += CSpecialProtocol::TranslatePath("special://xbmc/system/python/Lib/site-packages");
    std::wstring pythonPathW;
    CCharsetConverter::utf8ToW(pythonPath, pythonPathW);

    Py_SetPath(pythonPathW.c_str());

    Py_OptimizeFlag = 1;
#endif

    // *::GlobalInitializeModules() functions call PyImport_ExtendInittab(). PyImport_ExtendInittab() should
    // be called before Py_Initialize() as required by the Python documentation.
    CAddonPythonInvoker::GlobalInitializeModules();

#ifdef HAS_WEB_INTERFACE
    CHTTPPythonWsgiInvoker::GlobalInitializeModules();
#endif

    Py_Initialize();

#if PY_VERSION_HEX < 0x03070000
    // Python >= 3.7 Py_Initialize implicitly calls PyEval_InitThreads
    // Python < 3.7 we have to manually call initthreads.
    // PyEval_InitThreads is a no-op on subsequent calls, No need to wrap in
    // PyEval_ThreadsInitialized() check
    PyEval_InitThreads();
#endif

    // Acquire GIL if thread doesn't currently hold.
    if (!PyGILState_Check())
      PyEval_RestoreThread((PyThreadState*)m_mainThreadState);

    if (!(m_mainThreadState = PyThreadState_Get()))
      CLog::Log(LOGERROR, "Python threadstate is NULL.");
    savestate = PyEval_SaveThread();

    m_bInitialized = true;
  }

  return m_bInitialized;
}

void XBPython::OnScriptStarted(ILanguageInvoker* invoker)
{
  if (invoker == NULL)
    return;

  if (!m_bInitialized)
    return;

  PyElem inf;
  inf.id = invoker->GetId();
  inf.bDone = false;
  inf.pyThread = static_cast<CPythonInvoker*>(invoker);
  std::unique_lock<CCriticalSection> lock(m_vecPyList);
  m_vecPyList.push_back(inf);
}

void XBPython::NotifyScriptAborting(ILanguageInvoker* invoker)
{
  XBMC_TRACE;

  long invokerId(-1);
  if (invoker != NULL)
    invokerId = invoker->GetId();

  LOCK_AND_COPY(std::vector<XBMCAddon::xbmc::Monitor*>, tmp, m_vecMonitorCallbackList);
  for (auto& it : tmp)
  {
    if (CHECK_FOR_ENTRY(m_vecMonitorCallbackList, it))
    {
      if (invokerId < 0 || it->GetInvokerId() == invokerId)
        it->AbortNotify();
    }
  }
}

void XBPython::OnExecutionEnded(ILanguageInvoker* invoker)
{
  std::unique_lock<CCriticalSection> lock(m_vecPyList);
  PyList::iterator it = m_vecPyList.begin();
  while (it != m_vecPyList.end())
  {
    if (it->id == invoker->GetId())
    {
      if (it->pyThread->IsStopping())
        CLog::Log(LOGDEBUG, "Python interpreter interrupted by user");
      else
        CLog::Log(LOGDEBUG, "Python interpreter stopped");
      it->bDone = true;
    }
    ++it;
  }
}

void XBPython::OnScriptFinalized(ILanguageInvoker* invoker)
{
  XBMC_TRACE;
  std::unique_lock<CCriticalSection> lock(m_critSection);
  // for linux - we never release the library. its loaded and stays in memory.
  if (m_iDllScriptCounter)
    m_iDllScriptCounter--;
  else
    CLog::Log(LOGERROR, "Python script counter attempted to become negative");
}

ILanguageInvoker* XBPython::CreateInvoker()
{
  return new CAddonPythonInvoker(this);
}

void XBPython::PulseGlobalEvent()
{
  m_globalEvent.Set();
}

bool XBPython::WaitForEvent(CEvent& hEvent, unsigned int milliseconds)
{
  // wait for either this event our our global event
  XbmcThreads::CEventGroup eventGroup{&hEvent, &m_globalEvent};
  CEvent* ret = eventGroup.wait(std::chrono::milliseconds(milliseconds));
  if (ret)
    m_globalEvent.Reset();
  return ret != NULL;
}
