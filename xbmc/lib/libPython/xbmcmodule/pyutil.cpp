#include "stdafx.h"
#include "pyutil.h"
#include <wchar.h>
#include "stdstring.h"
#include "graphiccontext.h"

static int iPyGUILockRef = 0;

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

namespace PYXBMC
{
	int PyGetUnicodeString(wstring& buf, PyObject* pObject, int pos)
	{
		if(PyUnicode_Check(pObject))
		{
			buf = PyUnicode_AsUnicode(pObject);
			return 1;
		}
		if(PyString_Check(pObject))
		{
			buf = CStdStringW(PyString_AsString(pObject));
			return 1;
		}
		// object is not an unicode ar normal string
		buf = L"";
		if (pos != -1) PyErr_Format(PyExc_TypeError, "argument %.200i must be unicode or str", pos);
		return 0;
	}

	void PyGUILock()
	{
		if (iPyGUILockRef == 0) g_graphicsContext.Lock();
		iPyGUILockRef++;
	}

	void PyGUIUnlock()
	{
		if (iPyGUILockRef > 0)
		{
			iPyGUILockRef--;
			if (iPyGUILockRef == 0) g_graphicsContext.Unlock();	
		}
	}
}
