#pragma once

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

#include "XBPyThread.h"
#include "cores/IPlayer.h"
#include "utils/CriticalSection.h"

#include <vector>

typedef struct {
  int id;
  bool bDone;
  std::string strFile;
  XBPyThread *pyThread;
}PyElem;

class LibraryLoader;

typedef std::vector<PyElem> PyList;
typedef std::vector<PVOID> PlayerCallbackList;
typedef std::vector<LibraryLoader*> PythonExtensionLibraries;

class XBPython : public IPlayerCallback
{
public:
  XBPython();
  virtual ~XBPython();
  virtual void OnPlayBackEnded();
  virtual void OnPlayBackStarted();
  virtual void OnPlayBackPaused();
  virtual void OnPlayBackResumed();
  virtual void OnPlayBackStopped();
  virtual void OnQueueNextItem() {};
  void RegisterPythonPlayerCallBack(IPlayerCallback* pCallback);
  void UnregisterPythonPlayerCallBack(IPlayerCallback* pCallback);
  void Initialize();
  void Finalize();
  void FinalizeScript();
  void FreeResources();
  void Process();

  void PulseGlobalEvent();
  void WaitForEvent(HANDLE hEvent, unsigned int timeout);

  int ScriptsSize();
  int GetPythonScriptId(int scriptPosition);
  int evalFile(const CStdString &src);
  int evalFile(const CStdString &src, const std::vector<CStdString> &argv);
  int evalString(const CStdString &src, const std::vector<CStdString> &argv);

  bool isRunning(int scriptId);
  bool isStopping(int scriptId);
  void setDone(int id);
  
  /*! \brief Stop a script if it's running
   \param path path to the script
   \return true if the script was running and is now stopped, false otherwise
   */
  bool StopScript(const CStdString &path);

  // inject xbmc stuff into the interpreter.
  // should be called for every new interpreter
  void InitializeInterpreter();

  // remove modules and references when interpreter done
  void DeInitializeInterpreter();

  void RegisterExtensionLib(LibraryLoader *pLib);
  void UnregisterExtensionLib(LibraryLoader *pLib);
  void UnloadExtensionLibs();

  //only should be called from thread which is running the script
  void  stopScript(int scriptId);

  // returns NULL if script doesn't exist or if script doesn't have a filename
  const char* getFileName(int scriptId);

  // returns -1 if no scripts exist with specified filename
  int getScriptId(const CStdString &strFile);

  PyThreadState *getMainThreadState();

  bool m_bLogin;
  CCriticalSection    m_critSection;
private:
  bool              FileExist(const char* strFile);

  int               m_nextid;
  PyThreadState*    m_mainThreadState;
  ThreadIdentifier  m_ThreadId;
  bool              m_bInitialized;
  HANDLE            m_hEvent;
  int               m_iDllScriptCounter; // to keep track of the total scripts running that need the dll
  HMODULE           m_hModule;
  unsigned int      m_endtime;

  //Vector with list of threads used for running scripts
  PyList              m_vecPyList;
  PlayerCallbackList  m_vecPlayerCallbackList;
  LibraryLoader*      m_pDll;

  // any global events that scripts should be using
  HANDLE m_globalEvent;

  // in order to finalize and unload the python library, need to save all the extension libraries that are
  // loaded by it and unload them first (not done by finalize)
  PythonExtensionLibraries m_extensions;
};

extern XBPython g_pythonParser;
