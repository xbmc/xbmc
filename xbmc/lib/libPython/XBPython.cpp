#include "XBPython.h"

extern "C" {
	//xbmc module
	extern void initxbmc(void);

	// used for testing
	extern void free_arenas(void);
}

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

XBPython::XBPython()
{
	mainThreadState = NULL;
	nextid = 0;

	/* Initialize the Python interpreter.  Required. */
	Py_Initialize();
	PyEval_InitThreads();
	initxbmc();
	/* redirecting default output to debug console */
	if (PyRun_SimpleString(""
			"import xbmc\n"
			"class xbmcout:\n"
			"	def write(self, data):\n"
			"		xbmc.output(data)\n"
			"	def flush(self):\n"
			"		xbmc.output(\".\")\n"
			"	def close(self):\n"
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
	InitializeCriticalSection(&m_critSection);
}

XBPython::~XBPython()
{
	DeleteCriticalSection(&m_critSection);
	// shut down the interpreter
	PyEval_AcquireLock();
	PyThreadState_Swap(mainThreadState);
	Py_Finalize();
	// free_arenas();
}

int XBPython::evalFile(const char *src)
{
	if (!Py_IsInitialized()) return -1;
	nextid++;
	XBPyThread *pyThread = new XBPyThread(this, mainThreadState, nextid);
	pyThread->evalFile(src);
	PyElem inf;
	inf.id = nextid;
	inf.bDone = false;
	strcpy(inf.strFile, src);
	inf.pyThread = pyThread;

	EnterCriticalSection(&m_critSection );
	vecPyList.push_back(inf);
	LeaveCriticalSection(&m_critSection );

	return nextid;
}

int XBPython::evalString(const char *src)
{
	nextid++;
	if (!Py_IsInitialized()) return -1;
	XBPyThread *pyThread = new XBPyThread(this, mainThreadState, nextid);
	pyThread->evalString(src);
	//strcpy(strFile, "");
	//addToList(pyThread);
	return nextid;
}

bool XBPython::isDone(int id)
{
	bool bIsDone = true;
	EnterCriticalSection(&m_critSection );

	PyList::iterator it = vecPyList.begin();
	while (it != vecPyList.end())
	{
		if (it->id == id && !it->bDone) bIsDone = false;

		//delete scripts which are done
		if (it->bDone)
		{
			//wait 10 sec, should be enough for slow scripts :-)
			it->pyThread->WaitForThreadExit(10000); 
			delete it->pyThread;
			vecPyList.erase(it);
		}
		else ++it;
	}

	LeaveCriticalSection(&m_critSection );
	return bIsDone;
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

int XBPython::scriptsSize()
{
	int iSize;
	EnterCriticalSection(&m_critSection);

	iSize = vecPyList.size();

	LeaveCriticalSection(&m_critSection);
	return iSize;	
}

char* XBPython::getFileName(int scriptId)
{
	char* cFileName = NULL;
	EnterCriticalSection(&m_critSection);

	PyList::iterator it = vecPyList.begin();
	while (it != vecPyList.end())
	{
		if (it->id == scriptId) cFileName = it->strFile;
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
		if (!strcmp(it->strFile, strFile)) iId = it->id;
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
