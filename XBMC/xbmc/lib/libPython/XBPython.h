#pragma comment(linker, "/merge:PY_TEXT=PYTHON")
#pragma comment(linker, "/merge:PY_DATA=PY_RW")
#pragma comment(linker, "/merge:PY_BSS=PY_RW")
#pragma comment(linker, "/merge:PY_RDATA=PYTHON")
//#pragma comment(linker, "/section:PYTHON,RWE")

#ifndef XBPYTHON_H_
#define XBPYTHON_H_

#include <xtl.h>
#include "XBPyThread.h"
#include "OutputBuffer.h"
#include <vector>

VOID WINAPI PyThreadNotifyProc(BOOL fCreate);

typedef struct PyElem {
	int id;
	char strFile[1024];
	XBPyThread *pyThread;
}PyElem;

//Vector with list of threads used for running scripts
typedef std::vector<PyElem> PyList;

static PyList pyList;

static OutputBuffer *outputBuffer;

class XBPython
{
public:
	XBPython();
	virtual ~XBPython();
	int		evalFile(const char *);
	int		evalString(const char *);
	int		scriptsSize();
	bool	isRunning(int scriptId);
	bool	isStopping(int scriptId);
	bool	isDone(int scriptId);
	void	stopScript(int scriptId);

	// returns NULL if script doesn't exist or if script doesn't have a filename
	char*	getFileName(int scriptId);

	// returns -1 if no scripts exist with specified filename
	int		getScriptId(const char* strFile);

	void	enableOutput(int h, int l);
	int		getOutputHeight();
	int		getOutputLength();
	char*	getOutputLine(int nr);

	PyThreadState *getMainThreadState();

private:
	int nextid;
	PyThreadState *mainThreadState;
};	

#endif //XBPYTHON_H_