#ifndef XBPYTHREAD_H_
#define XBPYTHREAD_H_

#include "python.h"
#include "..\..\utils\Thread.h"

class XBPyThread : public CThread  
{
public:
	XBPyThread(LPVOID pExecuter, PyThreadState* mainThreadState, int id);
	virtual ~XBPyThread();
	int evalFile(const char*);
	int evalString(const char*);
	bool isDone();
	bool isStopping();
	void stop();
	
protected:
	PyThreadState*	threadState;
	LPVOID					pExecuter;

	char type;
	char *source;
	bool done;
	bool stopping;
	int id;

	void OnStartup();
	void Process();
	void OnExit();
};

#endif // XBPYTHREAD_H_