#include "..\python.h"
#include <string>

using namespace std;

namespace PYXBMC
{
	int PyGetUnicodeString(wstring& buf, PyObject* pObject, int pos = -1);
}
