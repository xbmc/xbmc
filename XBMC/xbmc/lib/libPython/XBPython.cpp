#include "XBPython.h"

XTHREAD_NOTIFICATION pyThreadNotification;

static PyObject *Py_XbOutput(PyObject *self, PyObject *args)
{
	char *s_line;
	if (!PyArg_ParseTuple(args, "s:andrelog", &s_line))	return NULL;

	//Py_BEGIN_ALLOW_THREADS
	if (outputBuffer == NULL) {
		OutputDebugString(s_line);
	} else { // print line to buffer
		OutputDebugString(s_line);
		outputBuffer->addLine(s_line);
	}
	//Py_END_ALLOW_THREADS

	Py_INCREF(Py_None);
	return Py_None;
}

//define c functions to be used in python here
static PyMethodDef xboxMethods[] = {
	{"xb_output", (PyCFunction)Py_XbOutput, METH_VARARGS, "xb_output(line) writes a message to debug terminal"},
	{NULL, NULL, 0, NULL}
};

XBPython::XBPython()
{
	outputBuffer = NULL;
	mainThreadState = NULL;
	nextid = 0;

	/* Initialize the Python interpreter.  Required. */
	Py_Initialize();
	PyEval_InitThreads();

	Py_InitModule("xbox", xboxMethods);

	/* redirecting default output to debug console */
	if (PyRun_SimpleString(""
			"import xbox\n"
			"class xboxout:\n"
			"	def write(self, data):\n"
			"		xbox.xb_output(data)\n"
			"	def flush(self):\n"
			"		xbox.xb_output(\".\")\n"
			"\n"
			"import sys\n"
			"sys.stdout = xboxout()\n"
			"sys.stderr = xboxout()\n"
			"print '-->Python Initialized<--'\n"
			"") == -1)
	{
		OutputDebugString("Python Initialize Error\n");
	}

	mainThreadState = PyThreadState_Get();
	// release the lock
	PyEval_ReleaseLock();

  // register notification routine with the system
  pyThreadNotification.pfnNotifyRoutine = PyThreadNotifyProc;
  XRegisterThreadNotifyRoutine( &pyThreadNotification, TRUE );
}

XBPython::~XBPython() {
	XRegisterThreadNotifyRoutine( &pyThreadNotification, FALSE );

	// shut down the interpreter
	PyEval_AcquireLock();
	PyThreadState_Swap(mainThreadState);
	Py_Finalize();
	//free_arenas();
	if (outputBuffer) delete outputBuffer;
}

int XBPython::evalFile(const char *src) {
	XBPyThread *pyThread = new XBPyThread(mainThreadState);
	pyThread->evalFile(src);
	PyElem inf;
	inf.id = nextid;
	strcpy(inf.strFile, src);
	inf.pyThread = pyThread;

	pyList.push_back(inf);
	return nextid++;
}

int XBPython::evalString(const char *src) {
	XBPyThread *pyThread = new XBPyThread(mainThreadState);
	pyThread->evalString(src);
	//strcpy(strFile, "");
	//addToList(pyThread);
	return nextid++;
}

bool XBPython::isDone(int id) {
	PyList::iterator it;
	for( it = pyList.begin(); it != pyList.end(); it++ ) {
		if (it->id == id) {
			return false;
		}
	}
	return true;
}

void XBPython::stopScript(int id) {
	OutputDebugString("Stopping script with id ");
	char *c = new char[10];
	c[0] = id + 48;
	c[1] = '\0';
	OutputDebugString(c);
	OutputDebugString("\n");

	PyList::iterator it;
	for( it = pyList.begin(); it != pyList.end(); it++ ) {
		if (it->id == id) {
			OutputDebugString("TODO threadstop()\n");
			it->pyThread->stop();
			return;
		}
	}
}

PyThreadState *XBPython::getMainThreadState() {
	return mainThreadState;
}

int XBPython::scriptsSize() {
	return pyList.size();
}

char* XBPython::getFileName(int scriptId)
{
	PyList::iterator it;
	for( it = pyList.begin(); it != pyList.end(); it++ )
	{
		if (it->id == scriptId) return it->strFile;
	}
	return NULL;
}

int XBPython::getScriptId(const char* strFile)
{
	PyList::iterator it;
	for( it = pyList.begin(); it != pyList.end(); it++ )
	{
		if (!strcmp(it->strFile, strFile)) return it->id;
	}
	return 0;
}

bool XBPython::isRunning(const char* strFile)
{
	PyList::iterator it;
	for( it = pyList.begin(); it != pyList.end(); it++ )
	{
		if (!strcmp(it->strFile, strFile)) return true;
	}
	return false;
}

int XBPython::getOutputHeight()
{
	if (outputBuffer) return outputBuffer->getHeight();
	else return 0;
}

int XBPython::getOutputLength()
{
	if (outputBuffer) return outputBuffer->getLength();
	else return 0;
}

char* XBPython::getOutputLine(int nr)
{
	if (outputBuffer) return outputBuffer->getLine(nr);
	else return NULL;
}

void XBPython::enableOutput(int h, int l)
{
	if (h > 0 && l > 0)	outputBuffer = new OutputBuffer(h, l);
}

VOID WINAPI PyThreadNotifyProc(BOOL fCreate) {
	if (!fCreate) {
		PyList::iterator it;
		for( it = pyList.begin(); it != pyList.end(); it++ ) {
			if (it->pyThread->isDone()) {
				//we have found the thread wich is done executing python code
				//thread didn't exit (wich is true because this method needs to complete first.
				//fixme?
				delete it->pyThread;
				OutputDebugString("Python script stopped\n");
				pyList.erase(it);
				return;
			}
		}
	}
}
