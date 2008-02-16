#include "../python/Python.h"

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

#ifdef __cplusplus
}
#endif
