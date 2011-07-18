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

#include <Python.h>

#include "guilib/GUISliderControl.h"
#include "control.h"
#include "pyutil.h"
#include "utils/log.h"
using namespace std;


#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  PyObject * ControlSlider_New (PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static const char* keywords[] = { "x", "y", "width", "height", "textureback", "texture", "texturefocus", NULL };
	
    ControlSlider *self;
    char *cTextureBack = NULL;
    char *cTexture = NULL;
    char *cTextureFoc  = NULL;
	
    self = (ControlSlider *) type->tp_alloc (type, 0);
    if (!self) return NULL;
    new(&self->strTextureBack) string();
    new(&self->strTexture) string();
    new(&self->strTextureFoc) string();    
	
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     (char*)"llll|sss",
                                     (char**)keywords,
                                     &self->dwPosX,
                                     &self->dwPosY,
                                     &self->dwWidth,
                                     &self->dwHeight,
                                     &cTextureBack,
                                     &cTexture,
                                     &cTextureFoc))
    {
      Py_DECREF( self );
      return NULL;
    }
    // if texture is supplied use it, else get default ones
    self->strTextureBack = cTextureBack ? cTextureBack : PyXBMCGetDefaultImage((char*)"slider", (char*)"texturesliderbar", (char*)"osd_slider_bg_2.png");
    self->strTexture = cTexture ? cTexture : PyXBMCGetDefaultImage((char*)"slider", (char*)"textureslidernib", (char*)"osd_slider_nibNF.png");
    self->strTextureFoc = cTextureFoc ? cTextureFoc : PyXBMCGetDefaultImage((char*)"slider", (char*)"textureslidernibfocus", (char*)"osd_slider_nib.png");
    
    return (PyObject*)self;
  }
  
  void ControlSlider_Dealloc(ControlSlider* self)
  {
    self->strTextureBack.~string();
    self->strTexture.~string();
    self->strTextureFoc.~string();
    self->ob_type->tp_free((PyObject*)self);
  }

  CGUIControl* ControlSlider_Create (ControlSlider* pControl)
  {
    
    pControl->pGUIControl = new CGUISliderControl(pControl->iParentId, pControl->iControlId,(float)pControl->dwPosX, (float)pControl->dwPosY,
              (float)pControl->dwWidth,(float)pControl->dwHeight,
              (CStdString)pControl->strTextureBack,(CStdString)pControl->strTexture,
              (CStdString)pControl->strTextureFoc,0);   
    
    
    return pControl->pGUIControl;
  }  

  PyDoc_STRVAR(getPercent__doc__,
    "getPercent() -- Returns a float of the percent of the slider.\n"
    "\n"
    "example:\n"
    "  - print self.slider.getPercent()\n");

  PyObject* ControlSlider_GetPercent(ControlSlider*self, PyObject *args)
  {
    if (self->pGUIControl)
    {
      float fPercent = (float)((CGUISliderControl*)self->pGUIControl)->GetPercentage();
      return Py_BuildValue((char*)"f", fPercent);
    }
    return Py_BuildValue((char*)"f", 0);
  }

  PyDoc_STRVAR(setPercent__doc__,
    "setPercent(50) -- Sets the percent of the slider.\n"
    "\n"
    "example:\n"
    "self.slider.setPercent(50)\n");

  PyObject* ControlSlider_SetPercent(ControlProgress *self, PyObject *args)
  {
    float fPercent = 0;
    if (!PyArg_ParseTuple(args, (char*)"f", &fPercent)) return NULL;
        
    if (self->pGUIControl)
      ((CGUISliderControl*)self->pGUIControl)->SetPercentage((int)fPercent);
        
    Py_INCREF(Py_None);
    return Py_None;
  }  
	
	
  PyMethodDef ControlSlider_methods[] = {
    {(char*)"getPercent", (PyCFunction)ControlSlider_GetPercent, METH_VARARGS, getPercent__doc__},
    {(char*)"setPercent", (PyCFunction)ControlSlider_SetPercent, METH_VARARGS, setPercent__doc__},  
    {NULL, NULL, 0, NULL}
  };

  // ControlProgress class
  PyDoc_STRVAR(ControlSlider__doc__,
   "ControlSlider class.\n"
   "\n"
   "ControlSlider(x, y, width, height[, textureback, texture, texturefocus])\n"
   "\n"
   "x              : integer - x coordinate of control.\n"
   "y              : integer - y coordinate of control.\n"
   "width          : integer - width of control.\n"
   "height         : integer - height of control.\n"
   "textureback    : [opt] string - image filename.\n"
   "texture        : [opt] string - image filename.\n"
   "texturefocus   : [opt] string - image filename.\n"               
   "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
   "       Once you use a keyword, all following arguments require the keyword.\n"
   "       After you create the control, you need to add it to the window with addControl().\n"
   "\n"
   "example:\n"
   "  - self.slider = xbmcgui.ControlSlider(100, 250, 350, 40)\n");

// Restore code and data sections to normal.

  PyTypeObject ControlSlider_Type;

  void initControlSlider_Type()
  {
    PyXBMCInitializeTypeObject(&ControlSlider_Type);

    ControlSlider_Type.tp_name = (char*)"xbmcgui.ControlSlider";
    ControlSlider_Type.tp_basicsize = sizeof(ControlSlider);
    ControlSlider_Type.tp_dealloc = (destructor)ControlSlider_Dealloc;
    ControlSlider_Type.tp_compare = 0;
    ControlSlider_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    ControlSlider_Type.tp_doc = ControlSlider__doc__;
    ControlSlider_Type.tp_methods = ControlSlider_methods;
    ControlSlider_Type.tp_base = &Control_Type;
    ControlSlider_Type.tp_new = ControlSlider_New;
  }
}

#ifdef __cplusplus
}
#endif
