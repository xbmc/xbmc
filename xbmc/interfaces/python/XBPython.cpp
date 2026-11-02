/*
 *  Copyright (C) 2005-2026 Team Kodi
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
#include "application/Application.h"
#include "filesystem/SpecialProtocol.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/legacy/AddonUtils.h"
#include "interfaces/legacy/CallbackHandler.h"
#include "interfaces/legacy/Monitor.h"
#include "interfaces/python/AddonPythonInvoker.h"
#include "interfaces/python/PythonInvoker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/JSONVariantWriter.h"
#include "utils/Variant.h"
#include "utils/log.h"

#ifdef HAS_WEB_INTERFACE
#include "network/httprequesthandler/python/HTTPPythonWsgiInvoker.h"
#endif

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <system_error>
#include <thread>

using namespace std::chrono_literals;

namespace
{
// how long to wait for the GIL once Kodi is stopping
constexpr auto GIL_WAIT = 5s;

struct GilWait
{
  std::mutex mutex;
  std::condition_variable cv;
  bool started{false};
  bool done{false};
  std::optional<bool> keep; // fn's result
  bool timedOut{false};

  // the GIL is kept until the process exits
  [[noreturn]] void Park(std::unique_lock<std::mutex>& lock)
  {
    while (true)
      cv.wait(lock);
  }
};
} // namespace

XBPython::XBPython()
{
  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this);

  CLog::Log(LOGDEBUG, "initializing python engine.");
  // Darwin packs .pyo files, we need PYTHONOPTIMIZE on in order to load them.
  // linux built with unified builds only packages the pyo files so need it
#if defined(TARGET_DARWIN) || defined(TARGET_LINUX)
  setenv("PYTHONOPTIMIZE", "1", 1);
#endif
  // Info about interesting python envvars available
  // at http://docs.python.org/using/cmdline.html#environment-variables

#if defined(TARGET_WASM)
  // CPython is linked statically here, so it has no install prefix to find on a
  // real filesystem. The standard library is preloaded into the Emscripten VFS
  // as <PYTHONHOME>/lib/pythonXY.zip, which is the zip landmark getpath() looks
  // for; see cmake/scripts/wasm/ExtraTargets.cmake.
  setenv("PYTHONHOME", CSpecialProtocol::TranslatePath("special://xbmc").c_str(), 1);
  CLog::Log(LOGDEBUG, "PYTHONHOME -> {}", CSpecialProtocol::TranslatePath("special://xbmc"));
#elif !defined(TARGET_WINDOWS) && !defined(TARGET_ANDROID)
  // check if we are running as real xbmc.app or just binary
  if (!CUtil::GetFrameworksPath(true).empty())
  {
    // using external python, its build looking for xxx/lib/python3.8
    // so point it to frameworks which is where python3.8 is located
    const auto pythonRootPath = CSpecialProtocol::TranslatePath("special://frameworks");
    setenv("PYTHONHOME", pythonRootPath.c_str(), 1);
    setenv("PYTHONPATH", pythonRootPath.c_str(), 1);
    CLog::Log(LOGDEBUG, "PYTHONHOME -> {}", pythonRootPath);
    CLog::Log(LOGDEBUG, "PYTHONPATH -> {}", pythonRootPath);
  }
#endif

  // *::GlobalInitializeModules() functions call PyImport_ExtendInittab(). PyImport_ExtendInittab() should
  // be called before Py_Initialize() as required by the Python documentation.
  CAddonPythonInvoker::GlobalInitializeModules();

#ifdef HAS_WEB_INTERFACE
  CHTTPPythonWsgiInvoker::GlobalInitializeModules();
#endif

  Py_Initialize();

  // a module adopts types shared with the others only on its first init, so
  // init them together here rather than in whichever sub-interpreter a script
  // first imports them from
  for (const auto& module : {"xbmc", "xbmcaddon", "xbmcdrm", "xbmcgui", "xbmcplugin", "xbmcvfs"
#ifdef HAS_WEB_INTERFACE
                             ,
                             "xbmcwsgi"
#endif
       })
  {
    PyObject* pyModule = PyImport_ImportModule(module);
    if (pyModule == nullptr)
    {
      CLog::Log(LOGFATAL, "python module {} failed to initialize", module);
      if (PyErr_Occurred())
        PyErr_Print();
      m_bindingModulesLoaded = false;
    }
    else
      Py_DECREF(pyModule);
  }

  m_mainThreadState = PyEval_SaveThread();
}

XBPython::~XBPython()
{
  XBMC_TRACE;
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);

  if (Py_IsInitialized())
  {
    // Uninitialize() kept the GIL for scripts or interpreters that are left, or it's lost
    if (m_skipFinalize || m_gilLost)
      return;

    // Switch to the main interpreter thread before finalizing
    PyThreadState_Swap(m_mainThreadState);

    // Clear all loaded modules to prevent circular references
    PyObject* modules = PyImport_GetModuleDict();
    PyDict_Clear(modules);

    Py_Finalize();
  }
}

#define LOCK_AND_COPY(type, dest, src) \
  std::unique_lock lock(src); \
  src.hadSomethingRemoved = false; \
  type dest; \
  dest = src

#define CHECK_FOR_ENTRY(l, v) (l.hadSomethingRemoved ? (std::ranges::find(l, v) != l.end()) : true)

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
  std::unique_lock lock(m_vecPlayerCallbackList);
  m_vecPlayerCallbackList.push_back(pCallback);
}

void XBPython::UnregisterPythonPlayerCallBack(IPlayerCallback* pCallback)
{
  XBMC_TRACE;
  std::unique_lock lock(m_vecPlayerCallbackList);
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
  std::unique_lock lock(m_vecMonitorCallbackList);
  m_vecMonitorCallbackList.push_back(pCallback);
}

void XBPython::UnregisterPythonMonitorCallBack(XBMCAddon::xbmc::Monitor* pCallback)
{
  XBMC_TRACE;
  std::unique_lock lock(m_vecMonitorCallbackList);
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

  // callbacks for the stopped scripts can still be queued
  XBMCAddon::RetardedAsyncCallbackHandler::clearAllPendingCalls();

  // a script that CScriptInvocationManager gave up on keeps its invoker, and a script that
  // failed can leave its interpreter behind: then keep the GIL from now on, so no further
  // Python code runs while Kodi exits, and don't finalize Python under them. Scripts not
  // counted by now are refused in OnScriptInitialized().
  int scriptsRunning;
  {
    std::unique_lock counterLock(m_critSection);
    if (m_scriptsClosed)
      return;
    m_scriptsClosed = true;
    scriptsRunning = static_cast<int>(m_scriptsStarted.size());
  }
  const auto keep = RunWithGil(
      [scriptsRunning]
      {
        for (PyInterpreterState* interp = PyInterpreterState_Head(); interp;
             interp = PyInterpreterState_Next(interp))
        {
          if (interp != PyInterpreterState_Main())
            return true;
        }
        return scriptsRunning > 0;
      });
  if (!keep)
  {
    CLog::Log(LOGWARNING,
              "XBPython: could not take the GIL, {} script(s) still running, not finalizing Python",
              scriptsRunning);
    m_skipFinalize = true;
    return;
  }
  if (scriptsRunning > 0)
    CLog::Log(LOGWARNING, "XBPython: {} script(s) still running, not finalizing Python",
              scriptsRunning);
  else if (*keep)
    CLog::Log(LOGWARNING, "XBPython: a script interpreter was not ended, not finalizing Python");
  m_skipFinalize = *keep;
}

std::optional<bool> XBPython::RunWithGil(const std::function<bool()>& fn)
{
  if (!g_application.IsStopping())
  {
    PyThreadState* ts = PyThreadState_New(PyInterpreterState_Main());
    if (ts == nullptr)
    {
      CLog::LogF(LOGERROR, "PyThreadState_New failed");
      return {};
    }
    PyEval_RestoreThread(ts);
    if (fn())
      return true;
    PyThreadState_Clear(ts);
    PyThreadState_DeleteCurrent();
    return false;
  }

  if (m_gilLost)
    return {};

  // A C call can hold the GIL for good, and there's no timed way to take it, so wait on a
  // helper thread. A helper that gets the GIL too late keeps it, so the script that held it
  // doesn't run on while Kodi is torn down. Its thread state is in the main interpreter, which
  // no script can free. fn only runs while the caller waits for it.
  auto wait = std::make_shared<GilWait>();
  try
  {
    std::thread(
        [wait, fn]
        {
          PyThreadState* ts = PyThreadState_New(PyInterpreterState_Main());
          if (ts != nullptr)
            PyEval_RestoreThread(ts);
          std::unique_lock lock(wait->mutex);
          if (ts == nullptr)
          {
            wait->done = true;
            wait->cv.notify_all();
            return;
          }
          if (wait->timedOut)
            wait->Park(lock);
          wait->started = true;
          wait->cv.notify_all();
          lock.unlock();
          const bool keep = fn();
          if (!keep)
          {
            PyThreadState_Clear(ts);
            PyThreadState_DeleteCurrent();
          }
          lock.lock();
          wait->done = true;
          wait->keep = keep;
          wait->cv.notify_all();
          if (keep)
            wait->Park(lock);
        })
        .detach();
  }
  catch (const std::system_error& e)
  {
    CLog::LogF(LOGERROR, "no thread to take the GIL: {}", e.what());
    return {};
  }

  std::unique_lock lock(wait->mutex);
  if (!wait->cv.wait_for(lock, GIL_WAIT, [&wait] { return wait->started || wait->done; }))
  {
    wait->timedOut = true;
    lock.unlock();
    // a caller on another thread can still start a helper until this is set; it times out too
    m_gilLost = true;
    CLog::Log(LOGERROR, "XBPython: could not take the GIL within {} s, not running any more Python",
              std::chrono::duration_cast<std::chrono::seconds>(GIL_WAIT).count());
    return {};
  }
  wait->cv.wait(lock, [&wait] { return wait->done; });
  if (!wait->keep)
    CLog::LogF(LOGERROR, "PyThreadState_New failed");
  return wait->keep;
}

void XBPython::Process()
{
  PyList tmpvec;
  std::unique_lock lock(m_vecPyList);
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

bool XBPython::OnScriptInitialized(ILanguageInvoker* invoker)
{
  if (invoker == NULL)
    return false;

  XBMC_TRACE;
  CLog::Log(LOGDEBUG, "initializing python script");
  std::unique_lock lock(m_critSection);
  if (m_scriptsClosed)
  {
    CLog::Log(LOGWARNING, "XBPython: Python is shutting down, not starting the script");
    return false;
  }
  m_scriptsStarted.insert(invoker);

  return true;
}

void XBPython::OnScriptStarted(ILanguageInvoker* invoker)
{
  if (invoker == NULL)
    return;

  PyElem inf;
  inf.id = invoker->GetId();
  inf.bDone = false;
  inf.pyThread = static_cast<CPythonInvoker*>(invoker);
  std::unique_lock lock(m_vecPyList);
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
  std::unique_lock lock(m_vecPyList);
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
  std::unique_lock lock(m_critSection);
  m_scriptsStarted.erase(invoker);
}

std::shared_ptr<ILanguageInvoker> XBPython::CreateInvoker()
{
  return std::make_shared<CAddonPythonInvoker>(this);
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
