#include "XBPyThread.h"
#include <signal.h>

int xbTrace(PyObject *obj, _frame *frame, int what, PyObject *arg)
{
	//return error, script will now be stopped.
	PyErr_SetString(PyExc_KeyboardInterrupt, "script interrupted by user\n");
	//PyErr_SetNone(PyExc_KeyboardInterrupt);
	return -1;
}

XBPyThread::XBPyThread(PyThreadState *mainThreadState)
{
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
	PyEval_AcquireLock();
	// swap in my thread state
	PyThreadState_Swap(threadState);

	if (type == 'F') {
		// run script from file
		FILE *fp = fopen(source, "r");
		if (fp)
		{
			if (PyRun_SimpleFile(fp, source) == -1)
			{
				OutputDebugString("Scriptresult: Error\n");
				if (PyErr_Occurred())
				{
					PyErr_Print();
				}
			}
			else OutputDebugString("Scriptresult: Succes\n");
			fclose(fp);
		}
		else pOut("%s not found!\n", source);
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
}

bool XBPyThread::isDone() {
	return done;
}

static void pytest(int err)
{

}

void XBPyThread::stop()
{
	PyEval_AcquireLock();

	// enable tracing. xbTrace will generate an error and the sript will be stopped
	//PyEval_SetTrace(xbTrace, NULL);

	//PyErr_SetInterrupt();
	threadState->c_tracefunc = xbTrace;
	threadState->use_tracing = 1;
	
	PyEval_ReleaseLock();
}
