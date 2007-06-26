#ifndef XBPYTHREAD_H_
#define XBPYTHREAD_H_

#ifndef _LINUX
#include "python/Python.h"
#else
#include <python2.4/Python.h>
#endif
#include "../../utils/Thread.h"

class XBPyThread : public CThread
{
public:
  XBPyThread(LPVOID pExecuter, PyThreadState* mainThreadState, int id);
  virtual ~XBPyThread();
  int evalFile(const char*);
  int evalString(const char*);
  int setArgv(const unsigned int, const char **);
  bool isDone();
  bool isStopping();
  void stop();

protected:
  PyThreadState*	m_threadState;
  PyThreadState*	m_mainThreadState;
  LPVOID		m_pExecuter;

  char type;
  char *source;
  char **argv;
  unsigned int  argc;
  bool done;
  bool stopping;
  int  m_id;

  virtual void OnStartup();
  virtual void Process();
  virtual void OnExit();
};

#endif // XBPYTHREAD_H_
