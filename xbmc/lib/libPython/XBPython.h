#pragma comment(linker, "/merge:PY_TEXT=PYTHON")
#pragma comment(linker, "/merge:PY_DATA=PY_RW")
#pragma comment(linker, "/merge:PY_BSS=PY_RW")
#pragma comment(linker, "/merge:PY_RDATA=PYTHON")

#pragma once

#include <xtl.h>
#include <vector>
#include "XBPyThread.h"
#include "IMsgSenderCallback.h"
#include "..\..\cores\IPlayer.h"

extern "C" {
	extern void initxbmc(void);
	extern void initxbmcgui(void);
	extern void free_arenas(void);
}

using namespace std;

typedef struct {
	int id;
	bool bDone;
	string strFile;
	XBPyThread *pyThread;
}PyElem;

typedef std::vector<PyElem> PyList;
typedef std::vector<PVOID> PlayerCallbackList;

class XBPython : public IMsgSenderCallback, public IPlayerCallback
{
public:
	      XBPython();
	virtual void SendMessage(CGUIMessage& message);
	virtual void OnPlayBackEnded();
	virtual void OnPlayBackStarted();
	virtual void OnPlayBackStopped();
	void	RegisterPythonPlayerCallBack(IPlayerCallback* pCallback);
	void	UnregisterPythonPlayerCallBack(IPlayerCallback* pCallback);
	void	Initialize();
	void	Finalize();
	void	FreeResources();
	void	Process();

	int		ScriptsSize();
	int		GetPythonScriptId(int scriptPosition); 
	int   evalFile(const char *);
	int   evalString(const char *);


	bool	isRunning(int scriptId);
	bool  isStopping(int scriptId);
	void	setDone(int id);

	//only should be called from thread which is running the script
	void  stopScript(int scriptId);

	// returns NULL if script doesn't exist or if script doesn't have a filename
	const char*	getFileName(int scriptId);

	// returns -1 if no scripts exist with specified filename
	int		getScriptId(const char* strFile);

  PyThreadState *getMainThreadState();

private:
  bool              FileExist(const char* strFile);
  
	int								nextid;
	PyThreadState*		mainThreadState;
	DWORD							dThreadId;
	bool							bInitialized;
	bool							bThreadInitialize;
	bool							bStartup;
	HANDLE						m_hEvent;

	//Vector with list of threads used for running scripts
	PyList vecPyList;
	PlayerCallbackList vecPlayerCallbackList;
	CRITICAL_SECTION	m_critSection;
};

extern XBPython g_pythonParser;