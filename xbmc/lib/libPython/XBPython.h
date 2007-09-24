#pragma once

#include "XBPyThread.h"
#include "IMsgSenderCallback.h"
#include "../../cores/IPlayer.h"

#include <vector>

typedef struct {
  int id;
  bool bDone;
  string strFile;
  XBPyThread *pyThread;
}PyElem;

typedef std::vector<PyElem> PyList;
typedef std::vector<PVOID> PlayerCallbackList;
typedef std::vector<LibraryLoader*> PythonExtensionLibraries;

class XBPython : public IMsgSenderCallback, public IPlayerCallback
{
public:
  XBPython();
  virtual bool SendMessage(CGUIMessage& message);
  virtual void OnPlayBackEnded();
  virtual void OnPlayBackStarted();
  virtual void OnPlayBackStopped();
  virtual void OnQueueNextItem() {};
  void	RegisterPythonPlayerCallBack(IPlayerCallback* pCallback);
  void	UnregisterPythonPlayerCallBack(IPlayerCallback* pCallback);
  void	Initialize();
  void	Finalize();
  void	FreeResources();
  void	Process();

  int	ScriptsSize();
  int	GetPythonScriptId(int scriptPosition);
  int   evalFile(const char *);
  int   evalFile(const char *, const unsigned int, const char **);

  bool	isRunning(int scriptId);
  bool  isStopping(int scriptId);
  void	setDone(int id);

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
  const char*	getFileName(int scriptId);

  // returns -1 if no scripts exist with specified filename
  int getScriptId(const char* strFile);

  PyThreadState *getMainThreadState();

  bool bStartup;
  bool bLogin;
private:
  bool              FileExist(const char* strFile);

  int               nextid;
  PyThreadState*    mainThreadState;
  DWORD             dThreadId;
  bool              m_bInitialized;
  HANDLE            m_hEvent;
  int               m_iDllScriptCounter; // to keep track of the total scripts running that need the dll
  HMODULE           m_hModule;

  //Vector with list of threads used for running scripts
  PyList vecPyList;
  PlayerCallbackList vecPlayerCallbackList;
  CRITICAL_SECTION	m_critSection;
  LibraryLoader*    m_pDll;

  // in order to finalize and unload the python library, need to save all the extension libraries that are
  // loaded by it and unload them first (not done by finalize)
  PythonExtensionLibraries m_extensions;
};

extern XBPython g_pythonParser;
