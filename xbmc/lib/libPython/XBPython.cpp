#include "stdafx.h"
#include "XBPython.h"
#include "..\..\sectionLoader.h"
#include "ActionManager.h"
#include "..\..\utils\log.h"

	 /* PY_RW stay's loaded as longs as m_pPythonParser != NULL.
	  * When someone runs a script for the first time both sections PYTHON and PY_RW
	  * are loaded. After that script has finished only section PYTHON is unloaded
	  * and m_pPythonParser is 'not' deleted.
		* Only delete m_pPythonParser and unload PY_RW if you don't want to use Python
		* anymore
		*/

XBPython g_pythonParser;

XBPython::XBPython()
{
	bInitialized = false;
	bThreadInitialize = false;
	bStartup = true;
	nextid = 0;
	mainThreadState = NULL;
	InitializeCriticalSection(&m_critSection);
	m_hEvent = CreateEvent(NULL, false, false, "pythonEvent");
	dThreadId = GetCurrentThreadId();
	vecPlayerCallbackList.clear();
}

void XBPython::SendMessage(CGUIMessage& message)
{
	evalFile(message.GetStringParam().c_str());
}

// message all registered callbacks that xbmc stopped playing
void XBPython::OnPlayBackEnded()
{
	if (!bInitialized) return;

	PlayerCallbackList::iterator it = vecPlayerCallbackList.begin();
	while (it != vecPlayerCallbackList.end())
	{
		((IPlayerCallback*)(*it))->OnPlayBackEnded();
		it++;
	}
}

// message all registered callbacks that we started playing
void XBPython::OnPlayBackStarted()
{
	if (!bInitialized) return;

	PlayerCallbackList::iterator it = vecPlayerCallbackList.begin();
	while (it != vecPlayerCallbackList.end())
	{
		((IPlayerCallback*)(*it))->OnPlayBackStarted();
		it++;
	}
}

// message all registered callbacks that user stopped playing
void XBPython::OnPlayBackStopped()
{
	if (!bInitialized) return;

	PlayerCallbackList::iterator it = vecPlayerCallbackList.begin();
	while (it != vecPlayerCallbackList.end())
	{
		((IPlayerCallback*)(*it))->OnPlayBackStopped();
		it++;
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

/**
 * Should be called before executing a script
 */
void XBPython::Initialize()
{
	g_sectionLoader.Load("PYTHON");
	if(!bInitialized)
	{
		if(dThreadId == GetCurrentThreadId())
		{
		  // first we check if all necessary files are installed
		  if (!FileExist("Q:\\python\\python23.zlib") ||
		      !FileExist("Q:\\python\\Lib\\_sre.pyd") ||
		      !FileExist("Q:\\python\\Lib\\_ssl.pyd") ||
		      !FileExist("Q:\\python\\Lib\\_symtable.pyd") ||
		      !FileExist("Q:\\python\\Lib\\pyexpat.pyd") ||
		      !FileExist("Q:\\python\\Lib\\unicodedata.pyd") ||
		      !FileExist("Q:\\python\\Lib\\zlib.pyd"))
		  {
		    CLog::Log(LOGERROR, "Python: Missing files, unable to execute script");
		    return;
		  }
		  
			g_sectionLoader.Load("PY_RW");

			Py_Initialize();
			PyEval_InitThreads();
			initxbmc(); // init xbmc modules
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
				OutputDebugString("Python Initialize Error\n");
			}

			mainThreadState = PyThreadState_Get();
			// release the lock
			PyEval_ReleaseLock();

			bInitialized = true;
			bThreadInitialize = false;
			PulseEvent(m_hEvent);
		}
		else
		{
			// only the main thread should initialize python, unload python again.
			g_sectionLoader.Unload("PYTHON");
			bThreadInitialize = true;
			WaitForSingleObject(m_hEvent, INFINITE);
		}
	}
}

/**
 * Should be called when a script is finished
 */
void XBPython::Finalize()
{
	g_sectionLoader.Unload("PYTHON");
}

void XBPython::FreeResources()
{
	if(bInitialized)
	{
		g_sectionLoader.Load("PYTHON");

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

		g_sectionLoader.Unload("PYTHON");
		g_sectionLoader.Unload("PY_RW");
	}
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
		evalFile("Q:\\scripts\\autoexec.py");
	}
	if (!bInitialized) return;

	EnterCriticalSection(&m_critSection );

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

// execute script, returns -1 if script doesn't exist
int XBPython::evalFile(const char *src)
{
	// return if file doesn't exist
	if(access(src, 0) == -1) return -1;

	Initialize();

  if (!bInitialized) return -1;

	nextid++;
	XBPyThread *pyThread = new XBPyThread(this, mainThreadState, nextid);
	pyThread->evalFile(src);
	PyElem inf;
	inf.id = nextid;
	inf.bDone = false;
	inf.strFile = src;
	inf.pyThread = pyThread;

	EnterCriticalSection(&m_critSection );
	vecPyList.push_back(inf);
	LeaveCriticalSection(&m_critSection );

	return nextid;
}

int XBPython::evalString(const char *src)
{
  Initialize();
  
  if (!bInitialized) return -1;
	/*
	nextid++;
	if (!Py_IsInitialized()) return -1;
	XBPyThread *pyThread = new XBPyThread(this, mainThreadState, nextid);
	pyThread->evalString(src);
	//strcpy(strFile, "");
	//addToList(pyThread);
	return nextid;
	*/
	return 0;
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
				OutputDebugString("Python script interrupted by user\n");
			else
				OutputDebugString("Python script stopped\n");
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
			Py_Output("Stopping script with id: %i\n", id);
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
		if (!strcmp(it->strFile.c_str(), strFile)) iId = it->id;
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