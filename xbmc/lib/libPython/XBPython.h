#pragma comment(linker, "/merge:PY_TEXT=PYTHON")
#pragma comment(linker, "/merge:PY_DATA=PY_RW")
#pragma comment(linker, "/merge:PY_BSS=PY_RW")
#pragma comment(linker, "/merge:PY_RDATA=PYTHON")
//#pragma comment(linker, "/section:PYTHON,RWE")

#ifndef XBPYTHON_H_
#define XBPYTHON_H_

#include <xtl.h>
#include <vector>

#include "XBPyThread.h"
#include "OutputBuffer.h"
#include "messages.h"

using namespace std;

VOID WINAPI PyThreadNotifyProc(BOOL fCreate);

typedef struct {
	int id;
	char strFile[1024];
	XBPyThread *pyThread;
}PyElem;

typedef std::vector<PyElem> PyList;

class XBPython
{
public:
	      XBPython();
	      ~XBPython();
	int   evalFile(const char *);
	int   evalString(const char *);
	int   scriptsSize();
	bool	isRunning(int scriptId);
	bool  isStopping(int scriptId);
	bool  isDone(int scriptId);
	void  stopScript(int scriptId);

	// returns NULL if script doesn't exist or if script doesn't have a filename
	char*	getFileName(int scriptId);

	// returns -1 if no scripts exist with specified filename
	int		getScriptId(const char* strFile);

	void	enableOutput(int h, int l);
	int		getOutputHeight();
	int		getOutputLength();
	char*	getOutputLine(int nr);

  PyThreadState *getMainThreadState();
	void setThreadNotification(bool bRegister);

	//Vector with list of threads used for running scripts
	PyList vecPyList;
	OutputBuffer *outputBuffer;
private:
	int nextid;
	PyThreadState *mainThreadState;
	bool bNotificating;
};	

#endif //XBPYTHON_H_