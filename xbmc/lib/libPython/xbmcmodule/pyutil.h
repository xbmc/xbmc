#include "..\python.h"
#include <string>

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

// credits and version information
#define PY_XBMC_AUTHOR		"J. Mulder <darkie@xboxmediacenter.com>"
#define PY_XBMC_CREDITS		"XBMC TEAM."

namespace PYXBMC
{
	int		PyGetUnicodeString(wstring& buf, PyObject* pObject, int pos = -1);
	void	PyGUILock();
	void	PyGUIUnlock();
}

#ifdef __cplusplus
}
#endif
