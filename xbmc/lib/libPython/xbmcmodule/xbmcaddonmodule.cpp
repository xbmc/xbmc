/*
 *      Copyright (C) 2005-2010 Team XBMC
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
  #include "lib/libPython/Python/Include/Python.h"
#endif

#include "PythonAddon.h"
#include "pyutil.h"

#ifndef __GNUC__
#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")
#endif

#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  /*****************************************************************
   * start of xbmcaddon methods
   *****************************************************************/
  // put module methods here


  // define c functions to be used in python here
  PyMethodDef xbmcAddonMethods[] = {
    {NULL, NULL, 0, NULL}
  };
  /*****************************************************************
   * end of methods and python objects
   *****************************************************************/

  PyMODINIT_FUNC
  InitAddonTypes(void)
  {
    initAddon_Type();

    if (PyType_Ready(&Addon_Type) < 0)
      return;
  }

  PyMODINIT_FUNC
  DeinitAddonModule(void)
  {
    // no need to Py_DECREF our objects (see InitAddonModule()) as they were created only
    // so that they could be added to the module, which steals a reference.
  }

  PyMODINIT_FUNC
  InitAddonModule(void)
  {
    Py_INCREF(&Addon_Type);

    // init general xbmcaddon modules
    PyObject* pXbmcAddonModule;
    pXbmcAddonModule = Py_InitModule((char*)"xbmcaddon", xbmcAddonMethods);
    if (pXbmcAddonModule == NULL) return;

    PyModule_AddObject(pXbmcAddonModule, (char*)"Addon", (PyObject*)&Addon_Type);

    // constants
    PyModule_AddStringConstant(pXbmcAddonModule, (char*)"__author__", (char*)PY_XBMC_AUTHOR);
    PyModule_AddStringConstant(pXbmcAddonModule, (char*)"__date__", (char*)"1 May 2010");
    PyModule_AddStringConstant(pXbmcAddonModule, (char*)"__version__", (char*)"1.0");
    PyModule_AddStringConstant(pXbmcAddonModule, (char*)"__credits__", (char*)PY_XBMC_CREDITS);
    PyModule_AddStringConstant(pXbmcAddonModule, (char*)"__platform__", (char*)PY_XBMC_PLATFORM);
  }
}

#ifdef __cplusplus
}
#endif
