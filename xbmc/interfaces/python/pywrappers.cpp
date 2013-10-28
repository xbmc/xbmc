/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifdef TARGET_LINUX

#include <Python.h>

#include "PyContext.h"

#include "interfaces/legacy/AddonUtils.h"
extern "C" {

  //  PyAPI_FUNC(PyGILState_STATE) __wrap_PyGILState_Ensure(void)
  PyAPI_FUNC(PyGILState_STATE) PyGILState_Ensure(void)
  {
    TRACE;
    XBMCAddon::Python::PyGILLock::acquireGil();
    return PyGILState_LOCKED;
  }

  //  PyAPI_FUNC(void) __wrap_PyGILState_Release(PyGILState_STATE)
  PyAPI_FUNC(void) PyGILState_Release(PyGILState_STATE)
  {
    TRACE;
    XBMCAddon::Python::PyGILLock::releaseGil();
  }
}

// endif TARGET_LINUX
#endif
