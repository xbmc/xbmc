#include "..\python.h"
#include <string>

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
	int		PyGetUnicodeString(wstring& buf, PyObject* pObject, int pos = -1);
	void	PyGUILock();
	void	PyGUIUnlock();
}

#ifdef __cplusplus
}
#endif
