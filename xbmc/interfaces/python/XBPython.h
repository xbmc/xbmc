#pragma once

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

#include "cores/IPlayerCallback.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/Thread.h"
#include "interfaces/IAnnouncer.h"
#include "interfaces/generic/ILanguageInvocationHandler.h"
#include "addons/IAddon.h"

#include <memory>
#include <vector>

class CPythonInvoker;

typedef struct {
  int id;
  bool bDone;
  CPythonInvoker* pyThread;
}PyElem;

class LibraryLoader;

namespace XBMCAddon
{
  namespace xbmc
  {
    class Monitor;
  }
}

template <class T> struct LockableType : public T, public CCriticalSection
{ bool hadSomethingRemoved; };

typedef LockableType<std::vector<void*> > PlayerCallbackList;
typedef LockableType<std::vector<XBMCAddon::xbmc::Monitor*> > MonitorCallbackList;
typedef LockableType<std::vector<PyElem> > PyList;
typedef std::vector<LibraryLoader*> PythonExtensionLibraries;

class XBPython :
  public IPlayerCallback,
  public ANNOUNCEMENT::IAnnouncer,
  public ILanguageInvocationHandler
{
public:
  XBPython();
  virtual ~XBPython();
  virtual void OnPlayBackEnded();
  virtual void OnPlayBackStarted();
  virtual void OnPlayBackPaused();
  virtual void OnPlayBackResumed();
  virtual void OnPlayBackStopped();
  virtual void OnPlayBackSpeedChanged(int iSpeed);
  virtual void OnPlayBackSeek(int iTime, int seekOffset);
  virtual void OnPlayBackSeekChapter(int iChapter);
  virtual void OnQueueNextItem();

  virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);
  void RegisterPythonPlayerCallBack(IPlayerCallback* pCallback);
  void UnregisterPythonPlayerCallBack(IPlayerCallback* pCallback);
  void RegisterPythonMonitorCallBack(XBMCAddon::xbmc::Monitor* pCallback);
  void UnregisterPythonMonitorCallBack(XBMCAddon::xbmc::Monitor* pCallback);
  void OnSettingsChanged(const std::string &strings);
  void OnScreensaverActivated();
  void OnScreensaverDeactivated();
  void OnDPMSActivated();
  void OnDPMSDeactivated();
  void OnScanStarted(const std::string &library);
  void OnScanFinished(const std::string &library);
  void OnCleanStarted(const std::string &library);
  void OnCleanFinished(const std::string &library);
  void OnAbortRequested(const std::string &ID="");
  void OnNotification(const std::string &sender, const std::string &method, const std::string &data);

  virtual void Process();
  virtual void Uninitialize();
  virtual void OnScriptStarted(ILanguageInvoker *invoker);
  virtual void OnScriptEnded(ILanguageInvoker *invoker);
  virtual ILanguageInvoker* CreateInvoker();

  bool InitializeEngine();
  void FinalizeScript();

  void PulseGlobalEvent();
  bool WaitForEvent(CEvent& hEvent, unsigned int milliseconds);

  void RegisterExtensionLib(LibraryLoader *pLib);
  void UnregisterExtensionLib(LibraryLoader *pLib);
  void UnloadExtensionLibs();

private:
  void Finalize();

  CCriticalSection    m_critSection;
  bool              FileExist(const char* strFile);

  void*             m_mainThreadState;
  ThreadIdentifier  m_ThreadId;
  bool              m_bInitialized;
  int               m_iDllScriptCounter; // to keep track of the total scripts running that need the dll
  unsigned int      m_endtime;

  //Vector with list of threads used for running scripts
  PyList              m_vecPyList;
  PlayerCallbackList  m_vecPlayerCallbackList;
  MonitorCallbackList m_vecMonitorCallbackList;
  LibraryLoader*      m_pDll;

  // any global events that scripts should be using
  CEvent m_globalEvent;

  // in order to finalize and unload the python library, need to save all the extension libraries that are
  // loaded by it and unload them first (not done by finalize)
  PythonExtensionLibraries m_extensions;
};

extern XBPython g_pythonParser;
