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

#ifdef _XBOX
#pragma comment(linker, "/merge:PY_TEXT=PYTHON")
#pragma comment(linker, "/merge:PY_DATA=PY_RW")
#pragma comment(linker, "/merge:PY_BSS=PY_RW")
#pragma comment(linker, "/merge:PY_RDATA=PYTHON")
#endif

// python.h should always be included first before any other includes
#include "stdafx.h"
#include "python/Python.h"
#include "cores/DllLoader/DllLoaderContainer.h"
#include "GUIPassword.h"

#include "XBPython.h"
#include "XBPythonDll.h"
#include "ActionManager.h"
#include "Settings.h"
#include "Profile.h"
#include "FileSystem/File.h"
#include "FileSystem/SpecialProtocol.h"
#include "xbox/network.h"
#include "Settings.h"

XBPython g_pythonParser;

#define PYTHON_DLL "Q:\\system\\python\\python24.dll"
#define PYTHON_LIBDIR "Q:\\system\\python\\lib\\"
#define PYTHON_EXT "Q:\\system\\python\\lib\\*.pyd"

extern "C" HMODULE __stdcall dllLoadLibraryA(LPCSTR file);
extern "C" BOOL __stdcall dllFreeLibrary(HINSTANCE hLibModule);


XBPython::XBPython()
{
  m_bInitialized = false;
  bThreadInitialize = false;
  bStartup = false;
  bLogin = false;
  nextid = 0;
  mainThreadState = NULL;
  InitializeCriticalSection(&m_critSection);
  m_hEvent = CreateEvent(NULL, false, false, (char*)"pythonEvent");
  dThreadId = GetCurrentThreadId();
  vecPlayerCallbackList.clear();
  m_iDllScriptCounter = 0;
}

XBPython::~XBPython()
{
}

bool XBPython::SendMessage(CGUIMessage& message)
{
  return (evalFile(message.GetStringParam().c_str()) != -1);
}

// message all registered callbacks that xbmc stopped playing
void XBPython::OnPlayBackEnded()
{
  if (m_bInitialized)
  {
    PlayerCallbackList::iterator it = vecPlayerCallbackList.begin();
    while (it != vecPlayerCallbackList.end())
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
    PlayerCallbackList::iterator it = vecPlayerCallbackList.begin();
    while (it != vecPlayerCallbackList.end())
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
    PlayerCallbackList::iterator it = vecPlayerCallbackList.begin();
    while (it != vecPlayerCallbackList.end())
    {
      ((IPlayerCallback*)(*it))->OnPlayBackStopped();
      it++;
    }
  }
}

void XBPython::RegisterPythonPlayerCallBack(IPlayerCallback* pCallback)
{
  vecPlayerCallbackList.push_back(pCallback);
}

void XBPython::UnregisterPythonPlayerCallBack(IPlayerCallback* pCallback)
{
  PlayerCallbackList::iterator it = vecPlayerCallbackList.begin();
  while (it != vecPlayerCallbackList.end())
  {
    if (*it == pCallback)
    {
      it = vecPlayerCallbackList.erase(it);
    }
    else it++;
  }
}

/**
* Check for file and print an error if needed
*/
bool XBPython::FileExist(const char* strFile)
{
  if (!strFile) return false;

  if (!XFILE::CFile::Exists(strFile))
  {
    CLog::Log(LOGERROR, "Python: Cannot find '%s'", strFile);
    return false;
  }
  return true;
}

/**
* Should be called before executing a script
*/
void XBPython::Initialize()
{
  CLog::Log(LOGINFO, "initializing python engine. ");
  EnterCriticalSection(&m_critSection);
  m_iDllScriptCounter++;
  if (!m_bInitialized)
  {
    if (dThreadId == GetCurrentThreadId())
    {
      m_hModule = dllLoadLibraryA(PYTHON_DLL);
      LibraryLoader* pDll = DllLoaderContainer::GetModule(m_hModule);
      if (!pDll || !python_load_dll(*pDll))
      {
        CLog::Log(LOGFATAL, "Python: error loading python24.dll");
        Finalize();
        return;
      }

      // first we check if all necessary files are installed
      if (!FileExist("Q:\\system\\python\\python24.zlib") ||
        !FileExist("Q:\\system\\python\\DLLs\\_socket.pyd") ||
        !FileExist("Q:\\system\\python\\DLLs\\_ssl.pyd") ||
        !FileExist("Q:\\system\\python\\DLLs\\bz2.pyd") ||
        !FileExist("Q:\\system\\python\\DLLs\\pyexpat.pyd") ||
        !FileExist("Q:\\system\\python\\DLLs\\select.pyd") ||
        !FileExist("Q:\\system\\python\\DLLs\\unicodedata.pyd") ||
        !FileExist("Q:\\system\\python\\DLLs\\zlib.pyd"))
      {
        CLog::Log(LOGERROR, "Python: Missing files, unable to execute script");
        Finalize();
        return;
      }

      Py_Initialize();
      PyEval_InitThreads();

      char* python_argv[1] = { (char*)"" } ;
      if (g_advancedSettings.m_bPythonVerbose)
        python_argv[1] = { (char*)"--verbose" } ;
        
      PySys_SetArgv(1, python_argv);

      initxbmc(); // init xbmc modules
      initxbmcplugin(); // init plugin modules
      initxbmcgui(); // init xbmcgui modules
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
        "print '-->Python Initialized<--'\n"
        "") == -1)
      {
        CLog::Log(LOGFATAL, "Python Initialize Error");
      }

      mainThreadState = PyThreadState_Get();
      // release the lock
      PyEval_ReleaseLock();

      m_bInitialized = true;
      bThreadInitialize = false;
      PulseEvent(m_hEvent);
    }
    else
    {
      // only the main thread should initialize python.
      m_iDllScriptCounter--;
      bThreadInitialize = true;

      LeaveCriticalSection(&m_critSection);
      WaitForSingleObject(m_hEvent, INFINITE);
      EnterCriticalSection(&m_critSection);
    }
  }
  LeaveCriticalSection(&m_critSection);
}

/**
* Should be called when a script is finished
*/
void XBPython::Finalize()
{
  EnterCriticalSection(&m_critSection);
  m_iDllScriptCounter--;
  if (m_iDllScriptCounter == 0 && m_bInitialized)
  {
    CLog::Log(LOGINFO, "Python, unloading python24.dll because no scripts are running anymore");
    PyEval_AcquireLock();
    PyThreadState_Swap(mainThreadState);
    Py_Finalize();
    // first free all dlls loaded by python, after that python24.dll (this is done by UnloadPythonDlls
    DllLoaderContainer::UnloadPythonDlls();
    m_hModule = NULL;
    mainThreadState = NULL;
    m_bInitialized = false;
  }
  LeaveCriticalSection(&m_critSection);
}

void XBPython::FreeResources()
{
  EnterCriticalSection(&m_critSection);
  if (m_bInitialized)
  {
    // cleanup threads that are still running
    PyList::iterator it = vecPyList.begin();
    while (it != vecPyList.end())
    {
      delete it->pyThread;
      it = vecPyList.erase(it);
      Finalize();
    }
  }
  LeaveCriticalSection(&m_critSection);

  if (m_hEvent)
    CloseHandle(m_hEvent);

  DeleteCriticalSection(&m_critSection);
}

void XBPython::Process()
{
  // initialize if init was called from another thread
  if (bThreadInitialize) Initialize();

  if (bStartup)
  {
    bStartup = false;
	
	// We need to make sure the network is up in case the start scripts require network
	g_network.WaitForSetup(10000);
    
	if (evalFile("special://home/scripts/autoexec.py") < 0)
      evalFile("Q:\\scripts\\autoexec.py");
  }

  if (bLogin)
  {
    bLogin = false;
    evalFile("P:\\scripts\\autoexec.py");
  }

  EnterCriticalSection(&m_critSection);
  if (m_bInitialized)
  {
    PyList::iterator it = vecPyList.begin();
    while (it != vecPyList.end())
    {
      //delete scripts which are done
      if (it->bDone)
      {
        delete it->pyThread;
        it = vecPyList.erase(it);
        Finalize();
      }
      else ++it;
    }
  }
  LeaveCriticalSection(&m_critSection );
}

int XBPython::evalFile(const char *src) { return evalFile(src, 0, NULL); }
// execute script, returns -1 if script doesn't exist
int XBPython::evalFile(const char *src, const unsigned int argc, const char ** argv)
{
  // return if file doesn't exist
  if (!XFILE::CFile::Exists(src))
    return -1;

  // check if locked
  if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].programsLocked() && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return -1;

  Initialize();

  if (!m_bInitialized) return -1;

  nextid++;
  XBPyThread *pyThread = new XBPyThread(this, mainThreadState, nextid);
  if (argv != NULL)
    pyThread->setArgv(argc, argv);
  pyThread->evalFile(src);
  PyElem inf;
  inf.id = nextid;
  inf.bDone = false;
  inf.strFile = src;
  inf.pyThread = pyThread;

  EnterCriticalSection(&m_critSection);
  vecPyList.push_back(inf);
  LeaveCriticalSection(&m_critSection);

  return nextid;
}

void XBPython::setDone(int id)
{
  EnterCriticalSection(&m_critSection);
  PyList::iterator it = vecPyList.begin();
  while (it != vecPyList.end())
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
  LeaveCriticalSection(&m_critSection);
}

void XBPython::stopScript(int id)
{
  EnterCriticalSection(&m_critSection);
  PyList::iterator it = vecPyList.begin();
  while (it != vecPyList.end())
  {
    if (it->id == id) {
      CLog::Log(LOGINFO, "Stopping script with id: %i", id);
      it->pyThread->stop();
      LeaveCriticalSection(&m_critSection);
      return;
    }
    ++it;
  }
  LeaveCriticalSection(&m_critSection );
}

PyThreadState *XBPython::getMainThreadState()
{
  return mainThreadState;
}

int XBPython::ScriptsSize()
{
  int iSize = 0;
  
  EnterCriticalSection(&m_critSection);
  iSize = vecPyList.size();
  LeaveCriticalSection(&m_critSection);

  return iSize;
}

const char* XBPython::getFileName(int scriptId)
{
  const char* cFileName = NULL;
  
  EnterCriticalSection(&m_critSection);
  PyList::iterator it = vecPyList.begin();
  while (it != vecPyList.end())
  {
    if (it->id == scriptId) cFileName = it->strFile.c_str();
    ++it;
  }
  LeaveCriticalSection(&m_critSection);

  return cFileName;
}

int XBPython::getScriptId(const char* strFile)
{
  int iId = -1;
  
  EnterCriticalSection(&m_critSection);
  PyList::iterator it = vecPyList.begin();
  while (it != vecPyList.end())
  {
    if (!stricmp(it->strFile.c_str(), strFile)) iId = it->id;
    ++it;
  }
  LeaveCriticalSection(&m_critSection);
  
  return iId;
}

bool XBPython::isRunning(int scriptId)
{
  bool bRunning = false;
  
  EnterCriticalSection(&m_critSection);
  PyList::iterator it = vecPyList.begin();
  while (it != vecPyList.end())
  {
    if (it->id == scriptId)	bRunning = true;
    ++it;
  }
  LeaveCriticalSection(&m_critSection);
  
  return bRunning;
}

bool XBPython::isStopping(int scriptId)
{
  bool bStopping = false;
  
  EnterCriticalSection(&m_critSection);
  PyList::iterator it = vecPyList.begin();
  while (it != vecPyList.end())
  {
    if (it->id == scriptId) bStopping = it->pyThread->isStopping();
    ++it;
  }
  LeaveCriticalSection(&m_critSection);
  
  return bStopping;
}

int XBPython::GetPythonScriptId(int scriptPosition)
{
  int iId = -1;

  EnterCriticalSection(&m_critSection);
  iId = (int)vecPyList[scriptPosition].id;
  LeaveCriticalSection(&m_critSection);

  return iId;
}
