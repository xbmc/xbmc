#include "stdafx.h"
#ifndef _LINUX
#include "../python/Python.h"
#else
#include <python2.4/Python.h>
#include "../XBPythonDll.h"
#endif
#include "GUILabelControl.h"
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
extern "C" {
#endif

namespace PYXBMC
{
  PyObject* ControlLabel_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = {
      "x", "y", "width", "height", "label", "font", "textColor",
      "disabledColor", "alignment", "hasPath", "angle", NULL };

    ControlLabel *self;
    char *cFont = NULL;
    char *cTextColor = NULL;
    char *cDisabledColor = NULL;
    PyObject* pObjectText;

    self = (ControlLabel*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    new(&self->strText) string();
    new(&self->strFont) string();

    // set up default values in case they are not supplied
    self->strFont = "font13";
    self->dwTextColor = 0xffffffff;
    self->dwDisabledColor = 0x60ffffff;
    self->dwAlign = XBFONT_LEFT;
    self->bHasPath = false;
    self->iAngle = 0;

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "llllO|ssslbl",
      keywords,
      &self->dwPosX,
      &self->dwPosY,
      &self->dwWidth,
      &self->dwHeight,
      &pObjectText,
      &cFont,
      &cTextColor,
      &cDisabledColor,
      &self->dwAlign,
      &self->bHasPath,
      &self->iAngle))
    {
        Py_DECREF( self );
        return NULL;
    }
    if (!PyGetUnicodeString(self->strText, pObjectText, 5))
    {
      Py_DECREF( self );
      return NULL;
    }

    if (cFont) self->strFont = cFont;
    if (cTextColor) sscanf(cTextColor, "%lx", &self->dwTextColor);
    if (cDisabledColor)
    {
      sscanf( cDisabledColor, "%lx", &self->dwDisabledColor );
    }

    return (PyObject*)self;
  }

  void ControlLabel_Dealloc(ControlLabel* self)
  {
    self->strText.~string();
    self->strFont.~string();
    self->ob_type->tp_free((PyObject*)self);
  }

  CGUIControl* ControlLabel_Create(ControlLabel* pControl)
  {
    CLabelInfo label;
    label.font = g_fontManager.GetFont(pControl->strFont);
    label.textColor = label.focusedColor = pControl->dwTextColor;
    label.disabledColor = pControl->dwDisabledColor;
    label.align = pControl->dwAlign;
    label.angle = (float)-pControl->iAngle;
    pControl->pGUIControl = new CGUILabelControl(
      pControl->iParentId,
      pControl->iControlId,
      (float)pControl->dwPosX,
      (float)pControl->dwPosY,
      (float)pControl->dwWidth,
      (float)pControl->dwHeight,
      pControl->strText,
      label,
      pControl->bHasPath);
    return pControl->pGUIControl;
  }

  // setLabel() Method
  PyDoc_STRVAR(setLabel__doc__,
    "setLabel(label) -- Set's text for this label.\n"
    "\n"
    "label          : string or unicode - text string.\n"
    "\n"
    "example:\n"
    "  - self.label.setLabel('Status')\n");

  PyObject* ControlLabel_SetLabel(ControlLabel *self, PyObject *args)
  {
    PyObject *pObjectText;

    if (!PyArg_ParseTuple(args, "O", &pObjectText))	return NULL;
    if (!PyGetUnicodeString(self->strText, pObjectText, 1)) return NULL;

    ControlLabel *pControl = (ControlLabel*)self;
    CGUIMessage msg(GUI_MSG_LABEL_SET, pControl->iParentId, pControl->iControlId);
    msg.SetLabel(self->strText);

    PyGUILock();
    if (pControl->pGUIControl) pControl->pGUIControl->OnMessage(msg);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // getLabel() Method
  PyDoc_STRVAR(getLabel__doc__,
    "getLabel() -- Returns the text value for this label.\n"
    "\n"
    "example:\n"
    "  - label = self.label.getLabel()\n");

  PyObject* ControlLabel_GetLabel(ControlLabel *self, PyObject *args)
  {
    if (!self->pGUIControl) return NULL;
    
    PyGUILock();
    const char *cLabel = self->strText.c_str();
    PyGUIUnlock();

    return Py_BuildValue("s", cLabel);
  }

  PyMethodDef ControlLabel_methods[] = {
    {"setLabel", (PyCFunction)ControlLabel_SetLabel, METH_VARARGS, setLabel__doc__},
    {"getLabel", (PyCFunction)ControlLabel_GetLabel, METH_VARARGS, getLabel__doc__},
    {NULL, NULL, 0, NULL}
  };

  // ControlLabel class
  PyDoc_STRVAR(controlLabel__doc__,
    "ControlLabel class.\n"
    "\n"
    "ControlLabel(x, y, width, height, label[, font, textColor, \n"
    "             disabledColor, alignment, hasPath, angle])\n"
    "\n"
    "x              : integer - x coordinate of control.\n"
    "y              : integer - y coordinate of control.\n"
    "width          : integer - width of control.\n"
    "height         : integer - height of control.\n"
    "label          : string or unicode - text string.\n"
    "font           : [opt] string - font used for label text. (e.g. 'font13')\n"
    "textColor      : [opt] hexstring - color of enabled label's label. (e.g. '0xFFFFFFFF')\n"
    "disabledColor  : [opt] hexstring - color of disabled label's label. (e.g. '0xFFFF3300')\n"
    "alignment      : [opt] integer - alignment of label - *Note, see xbfont.h\n"
    "hasPath        : [opt] bool - True=stores a path / False=no path.\n"
    "angle          : [opt] integer - angle of control. (+ rotates CCW, - rotates CW)"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "       After you create the control, you need to add it to the window with addControl().\n"
    "\n"
    "example:\n"
    "  - self.label = xbmcgui.ControlLabel(100, 250, 125, 75, 'Status', angle=45)\n");

  // Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject ControlLabel_Type;

  void initControlLabel_Type()
  {
    PyInitializeTypeObject(&ControlLabel_Type);

    ControlLabel_Type.tp_name = "xbmcgui.ControlLabel";
    ControlLabel_Type.tp_basicsize = sizeof(ControlLabel);
    ControlLabel_Type.tp_dealloc = (destructor)ControlLabel_Dealloc;
    ControlLabel_Type.tp_compare = 0;
    ControlLabel_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    ControlLabel_Type.tp_doc = controlLabel__doc__;
    ControlLabel_Type.tp_methods = ControlLabel_methods;
    ControlLabel_Type.tp_base = &Control_Type;
    ControlLabel_Type.tp_new = ControlLabel_New;
  }
}

#ifdef __cplusplus
}
#endif
