#ifndef XBPYTHREAD_H_
#define XBPYTHREAD_H_

#include "python.h"
#include "..\..\utils\Thread.h"

class XBPyThread : public CThread  
{
public:
	XBPyThread(PyThreadState *);
	virtual ~XBPyThread();
	int evalFile(const char *);
	int evalString(const char *);
	bool isDone();
	bool isStopping();
	void stop();
	
protected:
	PyThreadState *threadState;

	char type;
	char *source;
	bool done;
	bool stopping;

	void OnStartup();
	void Process();
	void OnExit();
};

#endif // XBPYTHREAD_H_