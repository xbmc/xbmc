#include "stdafx.h"
#ifndef _LINUX
#include "../python/Python.h"
#else
#include <python2.4/Python.h>
#include "../XBPythonDll.h"
#endif
#include "GUITextBox.h"
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
  extern PyObject* ControlSpin_New(void);

  PyObject* ControlTextBox_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = { "x", "y", "width", "height", "font", "textColor", NULL };
    ControlTextBox *self;
    char *cFont = NULL;
    char *cTextColor = NULL;

    self = (ControlTextBox*)type->tp_alloc(type, 0);
    if (!self) return NULL;

    self->pControlSpin = (ControlSpin*)ControlSpin_New();
    if (!self->pControlSpin) return NULL;
    new(&self->strFont) string();        

    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "llll|ss",
      keywords,
      &self->dwPosX,
      &self->dwPosY,
      &self->dwWidth,
      &self->dwHeight,
      &cFont,
      &cTextColor))
    {
      Py_DECREF( self );
      return NULL;
    }

    // set default values if needed
    self->strFont = cFont ? cFont : "font13";

    if (cTextColor) sscanf(cTextColor, "%lx", &self->dwTextColor);
    else self->dwTextColor = 0xffffffff;

    // default values for spin control
    self->pControlSpin->dwPosX = self->dwWidth - 25;
    self->pControlSpin->dwPosY = self->dwHeight - 30;

    return (PyObject*)self;
  }

  void ControlTextBox_Dealloc(ControlTextBox* self)
  {
    //Py_DECREF(self->pControlSpin);
    self->strFont.~string();       
    self->ob_type->tp_free((PyObject*)self);
  }

  CGUIControl* ControlTextBox_Create(ControlTextBox* pControl)
  {
    // create textbox
    CLabelInfo label;
    label.font = g_fontManager.GetFont(pControl->strFont);
    label.textColor = label.focusedColor = pControl->dwTextColor;
    CLabelInfo spinLabel;
    spinLabel.font = g_fontManager.GetFont(pControl->strFont);
    spinLabel.textColor = spinLabel.focusedColor = pControl->pControlSpin->dwColor;
    CImage up; up.file = pControl->pControlSpin->strTextureUp;
    CImage down; down.file = pControl->pControlSpin->strTextureDown;
    CImage upfocus; upfocus.file = pControl->pControlSpin->strTextureUpFocus;
    CImage downfocus; downfocus.file = pControl->pControlSpin->strTextureDownFocus;

    pControl->pGUIControl = new CGUITextBox(pControl->iParentId, pControl->iControlId,
      (float)pControl->dwPosX, (float)pControl->dwPosY, (float)pControl->dwWidth, (float)pControl->dwHeight,
      (float)pControl->pControlSpin->dwWidth, (float)pControl->pControlSpin->dwHeight,
      up, down, upfocus, downfocus, spinLabel, (float)pControl->pControlSpin->dwPosX,
      (float)pControl->pControlSpin->dwPosY, label);

    // reset textbox
    CGUIMessage msg(GUI_MSG_LABEL_RESET, pControl->iParentId, pControl->iControlId);
    pControl->pGUIControl->OnMessage(msg);

    // set values for spincontrol
    pControl->pControlSpin->iControlId = pControl->iControlId;
    pControl->pControlSpin->iParentId = pControl->iParentId;

    return pControl->pGUIControl;
  }

  // SetText() Method
  PyDoc_STRVAR(setText__doc__,
    "SetText(text) -- Set's the text for this textbox.\n"
    "\n"
    "text           : string or unicode - text string.\n"
    "\n"
    "example:\n"
    "  - self.textbox.SetText('This is a line of text that can wrap.')");


  PyObject* ControlTextBox_SetText(ControlTextBox *self, PyObject *args)
  {
    PyObject *pObjectText;
    string strText;
    if (!PyArg_ParseTuple(args, "O", &pObjectText))	return NULL;
    if (!PyGetUnicodeString(strText, pObjectText, 1)) return NULL;

    // create message
    ControlTextBox *pControl = (ControlTextBox*)self;
    CGUIMessage msg(GUI_MSG_LABEL_SET, pControl->iParentId, pControl->iControlId);
    msg.SetLabel(strText);

    // send message
    PyGUILock();
    if (pControl->pGUIControl) pControl->pGUIControl->OnMessage(msg);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // reset() Method
  PyDoc_STRVAR(reset__doc__,
    "reset() -- Clear's this textbox.\n"
    "\n"
    "example:\n"
    "  - self.textbox.reset()\n");

  PyObject* ControlTextBox_Reset(ControlTextBox *self, PyObject *args)
  {
    // create message
    ControlTextBox *pControl = (ControlTextBox*)self;
    CGUIMessage msg(GUI_MSG_LABEL_RESET, pControl->iParentId, pControl->iControlId);

    // send message
    PyGUILock();
    if (pControl->pGUIControl) pControl->pGUIControl->OnMessage(msg);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // getSpinControl() Method
  PyDoc_STRVAR(getSpinControl__doc__,
    "getSpinControl() -- Returns the associated ControlSpin."
    "\n"
    "- Not working completely yet -\n"
    "After adding this textbox to a window it is not possible to change\n"
    "the settings of this spin control."
    "\n"
    "example:\n"
    "  - id = self.textbox.getSpinControl()\n");

  PyObject* ControlTextBox_GetSpinControl(ControlTextBox *self, PyObject *args)
  {
    Py_INCREF(self->pControlSpin);
    return (PyObject*)self->pControlSpin;
  }

  PyMethodDef ControlTextBox_methods[] = {
    {"setText", (PyCFunction)ControlTextBox_SetText, METH_VARARGS, setText__doc__},
    {"reset", (PyCFunction)ControlTextBox_Reset, METH_VARARGS, reset__doc__},
    {"getSpinControl", (PyCFunction)ControlTextBox_GetSpinControl, METH_VARARGS, getSpinControl__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(controlTextBox__doc__,
    "ControlTextBox class.\n"
    "\n"
    "ControlTextBox(x, y, width, height[, font, textColor])\n"
    "\n"
    "x              : integer - x coordinate of control.\n"
    "y              : integer - y coordinate of control.\n"
    "width          : integer - width of control.\n"
    "height         : integer - height of control.\n"
    "font           : [opt] string - font used for text. (e.g. 'font13')\n"
    "textColor      : [opt] hexstring - color of textbox's text. (e.g. '0xFFFFFFFF')\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "       After you create the control, you need to add it to the window with addControl().\n"
    "\n"
    "example:\n"
    "  - self.textbox = xbmcgui.ControlTextBox(100, 250, 300, 300, textColor='0xFFFFFFFF')\n");

// Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject ControlTextBox_Type;

  void initControlTextBox_Type()
  {
    PyInitializeTypeObject(&ControlTextBox_Type);

    ControlTextBox_Type.tp_name = "xbmcgui.ControlTextBox";
    ControlTextBox_Type.tp_basicsize = sizeof(ControlTextBox);
    ControlTextBox_Type.tp_dealloc = (destructor)ControlTextBox_Dealloc;
    ControlTextBox_Type.tp_compare = 0;
    ControlTextBox_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    ControlTextBox_Type.tp_doc = controlTextBox__doc__;
    ControlTextBox_Type.tp_methods = ControlTextBox_methods;
    ControlTextBox_Type.tp_base = &Control_Type;
    ControlTextBox_Type.tp_new = ControlTextBox_New;
  }
}

#ifdef __cplusplus
}
#endif
