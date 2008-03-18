#ifndef _LINUX
#include "../python/Python.h"
#else
#include <python2.4/Python.h>
#include "../XBPythonDll.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// credits and version information
#define PY_XBMC_AUTHOR		"J. Mulder <darkie@xboxmediacenter.com>"
#define PY_XBMC_CREDITS		"XBMC TEAM."
#define PY_XBMC_PLATFORM	"XBOX"

namespace PYXBMC
{
  int   PyGetUnicodeString(std::string& buf, PyObject* pObject, int pos = -1);
  void  PyGUILock();
  void  PyGUIUnlock();
  const char* PyGetDefaultImage(char* controlType, char* textureType, char* cDefault);
  bool  PyWindowIsNull(void* pWindow);

  void  PyInitializeTypeObject(PyTypeObject* type_object);
}

#ifdef _LINUX

// Python under Linux for some reason doesn't play nice with Py_AddPendingCall
// and Py_MakePendingCalls, so we have our own versions.

#define Py_AddPendingCall _Py_AddPendingCall
#define Py_MakePendingCalls _Py_MakePendingCalls
  
void _Py_AddPendingCall(int(*func)(void*), void *arg);
void _Py_MakePendingCalls();
void PyInitPendingCalls();

#endif //_LINUX

#ifdef __cplusplus
}
#endif
