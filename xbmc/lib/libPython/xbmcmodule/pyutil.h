/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "lib/libPython/Python/Include/Python.h"
#include "../XBPythonDll.h"

#ifdef __cplusplus
extern "C" {
#endif

// credits and version information
#define PY_XBMC_AUTHOR		"J. Mulder <darkie@xbmc.org>"
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

// Python doesn't play nice with Py_AddPendingCall
// and Py_MakePendingCalls as it only allows them from
// the main python thread, which isn't what we want, so we have our own versions.

#define Py_AddPendingCall _Py_AddPendingCall
#define Py_MakePendingCalls _Py_MakePendingCalls
  
void _Py_AddPendingCall(int(*func)(void*), void *arg);
void _Py_MakePendingCalls();
void PyInitPendingCalls();

#ifdef __cplusplus
}
#endif
