#include "..\python.h"
#include <xtl.h>
#include <string>

using namespace std;

namespace PYXBMC
{
	int PyGetUnicodeString(wstring& buf, PyObject* pObject, int pos = -1);
}
