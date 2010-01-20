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
#include "GUIControlGroup.h"
#include "GUIFontManager.h"
#include "control.h"
#include "pyutil.h"

#ifndef __GNUC__
#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")
#endif

#ifdef __cplusplus
extern "C"
{
#endif
  namespace PYXBMC
  {
    PyObject * ControlGroup_New (PyTypeObject *type, PyObject *args, PyObject *kwds)
    {
      static const char *keywords[] = {
        "x", "y", "width", "height", NULL
      };

      ControlGroup *self;
      int ret;

      self = (ControlGroup *) type->tp_alloc (type, 0);
      if (!self) {
        return NULL;
      }

      ret = PyArg_ParseTupleAndKeywords (args,
                                         kwds,
                                         (char*)"llll",
                                         (char**)keywords,
                                         &self->dwPosX,
                                         &self->dwPosY,
                                         &self->dwWidth,
                                         &self->dwHeight);
      if (!ret) {
        Py_DECREF (self);
        return NULL;
      }

      return (PyObject *) self;
    }


    void ControlGroup_Dealloc (ControlGroup *self)
    {
      self->ob_type->tp_free ((PyObject *) self);
    }


    CGUIControl *
    ControlGroup_Create (ControlGroup *pCtrl)
    {
      pCtrl->pGUIControl = new CGUIControlGroup(pCtrl->iParentId,
                                                pCtrl->iControlId,
                                                (float) pCtrl->dwPosX,
                                                (float) pCtrl->dwPosY,
                                                (float) pCtrl->dwWidth,
                                                (float) pCtrl->dwHeight);
      return pCtrl->pGUIControl;
    }


    PyMethodDef ControlGroup_methods[] = {
      {NULL, NULL, 0, NULL}
    };


    // ControlGroup class
    PyDoc_STRVAR (controlGroup__doc__,
        "ControlGroup class.\n"
        "\n"
        "ControlGroup(x, y, width, height\n"
        "\n"
        "x              : integer - x coordinate of control.\n"
        "y              : integer - y coordinate of control.\n"
        "width          : integer - width of control.\n"
        "height         : integer - height of control.\n"
        "example:\n"
        "  - self.group = xbmcgui.ControlGroup(100, 250, 125, 75)\n");

// Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

    PyTypeObject ControlGroup_Type;

    void initControlGroup_Type ()
    {
      PyXBMCInitializeTypeObject (&ControlGroup_Type);

      ControlGroup_Type.tp_name = (char*)"xbmcgui.ControlGroup";
      ControlGroup_Type.tp_basicsize = sizeof (ControlGroup);
      ControlGroup_Type.tp_dealloc = (destructor) ControlGroup_Dealloc;
      ControlGroup_Type.tp_compare = 0;
      ControlGroup_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
      ControlGroup_Type.tp_doc = controlGroup__doc__;
      ControlGroup_Type.tp_methods = ControlGroup_methods;
      ControlGroup_Type.tp_base = &Control_Type;
      ControlGroup_Type.tp_new = ControlGroup_New;
    }
  }
#ifdef __cplusplus
}
#endif
