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

#include "utils/StdString.h"

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#if (defined USE_EXTERNAL_PYTHON)
  #if (defined HAVE_LIBPYTHON2_6)
    #include <python2.6/Python.h>
  #elif (defined HAVE_LIBPYTHON2_5)
    #include <python2.5/Python.h>
  #elif (defined HAVE_LIBPYTHON2_4)
    #include <python2.4/Python.h>
  #else
    #error "Could not determine version of Python to use."
  #endif
#else
  #include "python/Include/Python.h"
#endif
#include "../XBPythonDll.h"

#ifdef __cplusplus
extern "C" {
#endif

// credits and version information
#define PY_XBMC_AUTHOR    "J. Mulder <darkie@xbmc.org>"
#define PY_XBMC_CREDITS   "XBMC TEAM."
#define PY_XBMC_PLATFORM  "XBOX"

namespace PYXBMC
{
  int   PyXBMCGetUnicodeString(std::string& buf, PyObject* pObject, int pos = -1);
  void  PyXBMCGUILock();
  void  PyXBMCGUIUnlock();
  const char* PyXBMCGetDefaultImage(char* controlType, char* textureType, char* cDefault);
  bool  PyXBMCWindowIsNull(void* pWindow);

  void  PyXBMCInitializeTypeObject(PyTypeObject* type_object);
  void  PyXBMCWaitForThreadMessage(int message, int param1, int param2);
}

// Python doesn't play nice with PyXBMC_AddPendingCall
// and PyXBMC_MakePendingCalls as it only allows them from
// the main python thread, which isn't what we want, so we have our own versions.

#define PyXBMC_AddPendingCall _PyXBMC_AddPendingCall
#define PyXBMC_MakePendingCalls _PyXBMC_MakePendingCalls
#define PyXBMC_ClearPendingCalls _PyXBMC_ClearPendingCalls

void _PyXBMC_AddPendingCall(PyThreadState* state, int(*func)(void*), void *arg);
void _PyXBMC_MakePendingCalls();
void _PyXBMC_ClearPendingCalls(PyThreadState* state);

#ifdef __cplusplus
}
#endif
