
#include "../../stdafx.h"
#include "XBPyThread.h"
#include "XBPython.h"

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

int xbTrace(PyObject *obj, _frame *frame, int what, PyObject *arg)
{
	PyErr_SetString(PyExc_KeyboardInterrupt, "script interrupted by user\n");
	return -1;
}

XBPyThread::XBPyThread(LPVOID pExecuter, PyThreadState* mainThreadState, int id)
{
	this->pExecuter = pExecuter;
	this->id = id;
	// get the global lock
	PyEval_AcquireLock();
	// get a reference to the PyInterpreterState
	PyInterpreterState *mainInterpreterState = mainThreadState->interp;
	// create a thread state object for this thread
	threadState = PyThreadState_New(mainInterpreterState);

	// clear the thread state and free the lock
	PyThreadState_Swap(NULL);
	PyEval_ReleaseLock();

	done = false;
	stopping = false;
}

XBPyThread::~XBPyThread()
{
	if (source) delete []source;
}

int XBPyThread::evalFile(const char *src)
{
	type = 'F';
	source = new char[strlen(src)+1];
	strcpy(source, src);
	Create();
	return 0;
}

int XBPyThread::evalString(const char *src)
{
	type = 'S';
	source = new char[strlen(src)+1];
	strcpy(source, src);
	Create();
	return 0;
}

void XBPyThread::OnStartup(){}

void XBPyThread::Process()
{
	char path[1024];
	char sourcedir[1024];

	PyEval_AcquireLock();
	// swap in my thread state
	PyThreadState_Swap(threadState);
	//threadState-> >frame->
	// get path from script file name and add python path's
	// this is used for python so it will search modules from script path first
	strcpy(sourcedir, source);
	strcpy(strrchr(sourcedir, '\\'), ";");
	
	strcpy(path, sourcedir);
	strcat(path, getenv("PYTHONPATH"));

	// set current directory and python's path.
	PySys_SetPath(path);
	chdir(sourcedir);

	if (type == 'F') {
		// run script from file
		FILE *fp = fopen(source, "r");
		if (fp)
		{
			if (PyRun_SimpleFile(fp, source) == -1)
			{
				OutputDebugString("Scriptresult: Error\n");
				if (PyErr_Occurred())	PyErr_Print();
			}
			else OutputDebugString("Scriptresult: Succes\n");
			fclose(fp);
		}
		else Py_Output("%s not found!\n", source);
	}
	else
	{
		//run script
		if (PyRun_SimpleString(source) == -1)
		{
			OutputDebugString("Scriptresult: Error\n");
			if (PyErr_Occurred()) PyErr_Print();
		}
		else OutputDebugString("Scriptresult: Succes\n");
	}

	// clear the thread state and release our hold on the global interpreter
	PyThreadState_Swap(NULL);
	PyEval_ReleaseLock();
}

void XBPyThread::OnExit()
{
	// grab the lock
	PyEval_AcquireLock();
	// swap my thread state out of the interpreter
	PyThreadState_Swap(NULL);

	// clear out any cruft from thread state object
	PyThreadState_Clear(threadState);
	// delete my thread state object
	PyThreadState_Delete(threadState);
	// release the lock
	PyEval_ReleaseLock();

	done = true;
	((XBPython*)pExecuter)->setDone(id);
}

bool XBPyThread::isDone() {
	return done;
}

bool XBPyThread::isStopping() {
	return stopping;
}

void XBPyThread::stop()
{
	PyEval_AcquireLock();
	//PyErr_SetInterrupt();

	// enable tracing. xbTrace will generate an error and the sript will be stopped
	//PyEval_SetTrace(xbTrace, NULL);

	threadState->c_tracefunc = xbTrace;
	//arg threadState->c_traceobj
	threadState->use_tracing = 1;
	
	PyEval_ReleaseLock();

	stopping = true;
}
