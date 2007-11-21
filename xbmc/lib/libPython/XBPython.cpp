
#ifndef __GNUC__
#pragma comment(linker, "/merge:PY_TEXT=PYTHON")
#pragma comment(linker, "/merge:PY_DATA=PY_RW")
#pragma comment(linker, "/merge:PY_BSS=PY_RW")
#pragma comment(linker, "/merge:PY_RDATA=PYTHON")
#endif

// python.h should always be included first before any other includes
#include "stdafx.h"
#ifndef _LINUX
#include "python/Python.h"
#else
#include <python2.4/Python.h>
#endif
#include "../../cores/DllLoader/DllLoaderContainer.h"
#include "../../GUIPassword.h"
#include "../../Util.h"

#include "XBPython.h"
#include "XBPythonDll.h"
#include "ActionManager.h"

XBPython g_pythonParser;

#ifndef _LINUX
#define PYTHON_DLL "Q:\\system\\python\\python24.dll"
#define PYTHON_LIBDIR "Q:\\system\\python\\lib\\"
#define PYTHON_EXT "Q:\\system\\python\\lib\\*.pyd"
#else
#define PYTHON_DLL "Q:\\system\\python\\python24-i486-linux.so"
#endif

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
  m_bInitialized = false;
  bStartup = false;
  bLogin = false;
  nextid = 0;
  mainThreadState = NULL;
  InitializeCriticalSection(&m_critSection);
  m_hEvent = CreateEvent(NULL, false, false, "pythonEvent");
  dThreadId = GetCurrentThreadId();
  vecPlayerCallbackList.clear();
  m_iDllScriptCounter = 0;
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

  if (access(strFile, 0) != 0)
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

  CLog::Log(LOGDEBUG,"%s, adding %s (0x%x)", __FUNCTION__, pLib->GetName(), pLib);
  EnterCriticalSection(&m_critSection);
  m_extensions.push_back(pLib);
  LeaveCriticalSection(&m_critSection);
}

void XBPython::UnregisterExtensionLib(LibraryLoader *pLib)
{
  if (!pLib) 
    return;

  CLog::Log(LOGDEBUG,"%s, removing %s (0x%x)", __FUNCTION__, pLib->GetName(), pLib);
  EnterCriticalSection(&m_critSection);
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
  LeaveCriticalSection(&m_critSection);
}

void XBPython::UnloadExtensionLibs()
{
  CLog::Log(LOGDEBUG,"%s, clearing python extension libraries", __FUNCTION__);
  EnterCriticalSection(&m_critSection);
  PythonExtensionLibraries::iterator iter = m_extensions.begin();
  while (iter != m_extensions.end())
  {
      DllLoaderContainer::ReleaseModule(*iter);
      iter++;
  }

  m_extensions.clear();
  LeaveCriticalSection(&m_critSection);
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
  m_iDllScriptCounter++;
  EnterCriticalSection(&m_critSection);
  if (!m_bInitialized)
  {
    if (dThreadId == GetCurrentThreadId())
    {
#ifndef _LINUX
      //DllLoader* pDll = g_sectionLoader.LoadDLL(PYTHON_DLL);
      m_hModule = dllLoadLibraryA(PYTHON_DLL);
      m_pDll = DllLoaderContainer::GetModule(m_hModule);
#else
      m_pDll = DllLoaderContainer::LoadModule(PYTHON_DLL, NULL, true);
#endif
      if (!m_pDll || !python_load_dll(*m_pDll))
      {
        CLog::Log(LOGFATAL, "Python: error loading python24.dll");
        Finalize();
        return;
      }

      // first we check if all necessary files are installed
#ifndef _LINUX      
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
#endif        

#ifdef _LINUX
      // Required for python to find optimized code (pyo) files
      setenv("PYTHONOPTIMIZE", "1", 1);
#endif

      Py_Initialize();
      PyEval_InitThreads();

      //char* python_argv[1] = { "--verbose" } ;
      char* python_argv[1] = { "" } ;
      PySys_SetArgv(1, python_argv);

      InitXBMCTypes();
      InitGUITypes();
      InitPluginTypes();

      mainThreadState = PyThreadState_Get();

      // release the lock
      PyEval_ReleaseLock();

      m_bInitialized = true;
      PulseEvent(m_hEvent);
    }
    else
    {
      // only the main thread should initialize python.
      m_iDllScriptCounter--;

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
  m_iDllScriptCounter--;
  // for linux - we never release the library. its loaded and stays in memory.
  EnterCriticalSection(&m_critSection);
  if (m_iDllScriptCounter == 0 && m_bInitialized)
  {
    CLog::Log(LOGINFO, "Python, unloading python24.dll cause no scripts are running anymore");
    PyEval_AcquireLock();
    PyThreadState_Swap(mainThreadState);
    Py_Finalize();

    UnloadExtensionLibs();

    //g_sectionLoader.UnloadDLL(PYTHON_DLL);
    // first free all dlls loaded by python, after that python24.dll (this is done by UnloadPythonDlls
    //dllFreeLibrary(m_hModule);
    DllLoaderContainer::UnloadPythonDlls();
#ifdef _LINUX
    DllLoaderContainer::ReleaseModule(m_pDll);
#endif    
    m_hModule = NULL;

    m_bInitialized = false;
  }
  LeaveCriticalSection(&m_critSection);
}

void XBPython::FreeResources()
{
  if (m_bInitialized)
  {
    // cleanup threads that are still running
    EnterCriticalSection(&m_critSection );
    PyList::iterator it = vecPyList.begin();
    while (it != vecPyList.end())
    {
      //it->pyThread->stop();
      // seems the current way can't kill all running scripts
      // need some other way to do it. For now we don't wait to long

      // wait 1 sec, should be enough for slow scripts :-)
      //if(!it->pyThread->WaitForThreadExit(1000))
      //{
      // thread did not end, just kill it
      //}

      delete it->pyThread;
      it = vecPyList.erase(it);
      Finalize();
    }

    LeaveCriticalSection(&m_critSection );

    // shut down the interpreter
    PyEval_AcquireLock();
    PyThreadState_Swap(mainThreadState);
    Py_Finalize();
    // free_arenas();
    mainThreadState = NULL;
    g_sectionLoader.UnloadDLL(PYTHON_DLL);
  }

  CloseHandle(m_hEvent);
  DeleteCriticalSection(&m_critSection);
}

void XBPython::Process()
{
  if (bStartup)
  {
    bStartup = false;
    evalFile("Q:\\scripts\\autoexec.py");
  }

  if (bLogin)
  {
    bLogin = false;
    evalFile("P:\\scripts\\autoexec.py");
  }

  if (m_bInitialized)
  {
    EnterCriticalSection(&m_critSection);
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
    LeaveCriticalSection(&m_critSection );
  }

}

int XBPython::evalFile(const char *src) { return evalFile(src, 0, NULL); }
// execute script, returns -1 if script doesn't exist
int XBPython::evalFile(const char *src, const unsigned int argc, const char ** argv)
{
  CStdString srcStr = _P(src);
  
  // return if file doesn't exist
  if(access(srcStr.c_str(), 0) == -1) return -1;

  // check if locked
  if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].programsLocked() && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return -1;

  Initialize();

  if (!m_bInitialized) return -1;

  nextid++;
  XBPyThread *pyThread = new XBPyThread(this, nextid);
  if (argv != NULL)
    pyThread->setArgv(argc, argv);
  pyThread->evalFile(srcStr.c_str());
  PyElem inf;
  inf.id = nextid;
  inf.bDone = false;
  inf.strFile = srcStr.c_str();
  inf.pyThread = pyThread;

  EnterCriticalSection(&m_critSection );
  vecPyList.push_back(inf);
  LeaveCriticalSection(&m_critSection );

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
  int iSize;
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
  return (int)vecPyList[scriptPosition].id;
}
