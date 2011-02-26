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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#if (defined USE_EXTERNAL_PYTHON)
#include <Python.h>
#else
  #include "python/Include/Python.h"
#endif
#include "../XBPythonDll.h"
#include "listitem.h"

#pragma once

class CAction;

// python type checking
#define Action_Check(op) PyObject_TypeCheck(op, &Action_Type)
#define Action_CheckExact(op) ((op)->ob_type == &Action_Type)

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  typedef struct {
    PyObject_HEAD
    long id;
    float fAmount1;
    float fAmount2;
    float fRepeat;
    unsigned long buttonCode;
    std::string strAction;
  } Action;

  extern PyTypeObject Action_Type;

  PyObject* Action_FromAction(const CAction& action);

  void initAction_Type();

}

#ifdef __cplusplus
}
#endif
