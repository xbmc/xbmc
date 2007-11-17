#include "stdafx.h"
#ifndef _LINUX
#include "../python/Python.h"
#else
#include <python2.4/Python.h>
#include "../XBPythonDll.h"
#endif
#include "GUIButtonControl.h"
#include "GUIFontManager.h"
#include "control.h"
#include "pyutil.h"

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  PyObject* ControlButton_New(
    PyTypeObject *type,
    PyObject *args,
    PyObject *kwds )
  {
    static char *keywords[] = {
      "x", "y", "width", "height", "label",
      "focusTexture", "noFocusTexture",
      "textXOffset", "textYOffset", "alignment",
      "font", "textColor", "disabledColor", "angle", "shadowColor", "focusedColor", NULL };
    ControlButton *self;
    char* cFont = NULL;
    char* cTextureFocus = NULL;
    char* cTextureNoFocus = NULL;
    char* cTextColor = NULL;
    char* cDisabledColor = NULL;
    char* cShadowColor = NULL;
    char* cFocusedColor = NULL;

    PyObject* pObjectText;

    self = (ControlButton*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    new(&self->strFont) string();    
    new(&self->strText) string();    
    new(&self->strText2) string();    
    new(&self->strTextureFocus) string();    
    new(&self->strTextureNoFocus) string(); 
    
    // set up default values in case they are not supplied
    self->dwTextXOffset = CONTROL_TEXT_OFFSET_X;
    self->dwTextYOffset = CONTROL_TEXT_OFFSET_Y;
    self->dwAlign = (XBFONT_LEFT | XBFONT_CENTER_Y);
    self->strFont = "font13";
    self->dwTextColor = 0xffffffff;
    self->dwDisabledColor = 0x60ffffff;
    self->iAngle = 0;
    self->dwShadowColor = 0;
    self->dwFocusedColor = 0xffffffff;

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "llllO|sslllssslss",
      keywords,
      &self->dwPosX,
      &self->dwPosY,
      &self->dwWidth,
      &self->dwHeight,
      &pObjectText,
      &cTextureFocus,
      &cTextureNoFocus,
      &self->dwTextXOffset,
      &self->dwTextYOffset,
      &self->dwAlign,
      &cFont,
      &cTextColor,
      &cDisabledColor,
      &self->iAngle,
      &cShadowColor,
      &cFocusedColor))
    {
      Py_DECREF( self );
      return NULL;
    }


    if (!PyGetUnicodeString(self->strText, pObjectText, 5))
    {
      Py_DECREF( self );
      return NULL;
    }

    // if texture is supplied use it, else get default ones
    self->strTextureFocus = cTextureFocus ?
      cTextureFocus :
      PyGetDefaultImage("button", "texturefocus", "button-focus.png");
    self->strTextureNoFocus = cTextureNoFocus ?
      cTextureNoFocus :
      PyGetDefaultImage("button", "texturenofocus", "button-nofocus.jpg");

    if (cFont) self->strFont = cFont;
    if (cTextColor) sscanf( cTextColor, "%x", &self->dwTextColor );
    if (cDisabledColor) sscanf( cDisabledColor, "%x", &self->dwDisabledColor );
    if (cShadowColor) sscanf( cShadowColor, "%x", &self->dwShadowColor );
    if (cFocusedColor) sscanf( cFocusedColor, "%x", &self->dwFocusedColor );
    return (PyObject*)self;
  }

  void ControlButton_Dealloc(ControlButton* self)
  {
    self->strFont.~string();
    self->strText.~string();
    self->strText2.~string();
    self->strTextureFocus.~string();
    self->strTextureNoFocus.~string();
    self->ob_type->tp_free((PyObject*)self);
  }

  CGUIControl* ControlButton_Create(ControlButton* pControl)
  {
    CLabelInfo label;
    label.font = g_fontManager.GetFont(pControl->strFont);
    label.textColor = pControl->dwTextColor;
    label.disabledColor = pControl->dwDisabledColor;
    label.shadowColor = pControl->dwShadowColor;
    label.focusedColor = pControl->dwFocusedColor;
    label.align = pControl->dwAlign;
    label.offsetX = (float)pControl->dwTextXOffset;
    label.offsetY = (float)pControl->dwTextYOffset;
    label.angle = (float)-pControl->iAngle;
    pControl->pGUIControl = new CGUIButtonControl(
      pControl->iParentId,
      pControl->iControlId,
      (float)pControl->dwPosX,
      (float)pControl->dwPosY,
      (float)pControl->dwWidth,
      (float)pControl->dwHeight,
      (CStdString)pControl->strTextureFocus,
      (CStdString)pControl->strTextureNoFocus,
      label);

    CGUIButtonControl* pGuiButtonControl =
      (CGUIButtonControl*)pControl->pGUIControl;

    pGuiButtonControl->SetLabel(pControl->strText);
    pGuiButtonControl->SetLabel2(pControl->strText2);

    return pControl->pGUIControl;
  }

  // setDisabledColor() Method
  PyDoc_STRVAR(setDisabledColor__doc__,
    "setDisabledColor(disabledColor) -- Set's this buttons disabled color.\n"
    "\n"
    "disabledColor  : hexstring - color of disabled button's label. (e.g. '0xFFFF3300')\n"
    "\n"
    "example:\n"
    "  - self.button.setDisabledColor('0xFFFF3300')\n");

  PyObject* ControlButton_SetDisabledColor(ControlButton *self, PyObject *args)
  {
    char *cDisabledColor = NULL;

    if (!PyArg_ParseTuple(args, "s", &cDisabledColor))  return NULL;

    // ControlButton *pControl = (ControlButton*)self;

    if (cDisabledColor) sscanf(cDisabledColor, "%x", &self->dwDisabledColor);

    PyGUILock();
    if (self->pGUIControl)
    {
      ((CGUIButtonControl*)self->pGUIControl)->PythonSetDisabledColor(self->dwDisabledColor);
    }
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // setLabel() Method
  PyDoc_STRVAR(setLabel__doc__,
    "setLabel(label[, font, textColor, disabledColor, shadowColor, focusedColor]) -- Set's this buttons text attributes.\n"
    "\n"
    "label          : string or unicode - text string.\n"
    "font           : [opt] string - font used for label text. (e.g. 'font13')\n"
    "textColor      : [opt] hexstring - color of enabled button's label. (e.g. '0xFFFFFFFF')\n"
    "disabledColor  : [opt] hexstring - color of disabled button's label. (e.g. '0xFFFF3300')\n"
    "shadowColor    : [opt] hexstring - color of button's label's shadow. (e.g. '0xFF000000')\n"
    "focusedColor   : [opt] hexstring - color of focused button's label. (e.g. '0xFFFFFF00')\n"
    "label2         : [opt] string or unicode - text string.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - self.button.setLabel('Status', 'font14', '0xFFFFFFFF', '0xFFFF3300', '0xFF000000')\n");

  PyObject* ControlButton_SetLabel(ControlButton *self, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = {
      "label",
      "font",
      "textColor",
      "disabledColor",
      "shadowColor",
      "focusedColor",
      "label2",
      NULL};
    char *cFont = NULL;
    char *cTextColor = NULL;
    char *cDisabledColor = NULL;
    char *cShadowColor = NULL;
    char *cFocusedColor = NULL;
    PyObject *pObjectText = NULL;
    PyObject *pObjectText2 = NULL;
 
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "O|sssssO",
      keywords,
      &pObjectText,
      &cFont,
      &cTextColor,
      &cDisabledColor,
      &cShadowColor,
      &cFocusedColor,
      &pObjectText2))
    {
      return NULL;
    }
    if (!PyGetUnicodeString(self->strText, pObjectText, 1))
    {
      return NULL;
    }
    if (pObjectText2)
      PyGetUnicodeString(self->strText2, pObjectText2, 1);

    if (cFont) self->strFont = cFont;
    if (cTextColor) sscanf(cTextColor, "%x", &self->dwTextColor);
    if (cDisabledColor) sscanf( cDisabledColor, "%x", &self->dwDisabledColor );
    if (cShadowColor) sscanf(cShadowColor, "%x", &self->dwShadowColor);
    if (cFocusedColor) sscanf(cFocusedColor, "%x", &self->dwFocusedColor);

    PyGUILock();
    if (self->pGUIControl)
    {
      ((CGUIButtonControl*)self->pGUIControl)->PythonSetLabel(
        self->strFont, self->strText, self->dwTextColor, self->dwShadowColor, self->dwFocusedColor );
      // hack for now - for some reason strText2 isnt allocated. causes a crash
      if (self->strText2.c_str())
        ((CGUIButtonControl*)self->pGUIControl)->SetLabel2(self->strText2);
      ((CGUIButtonControl*)self->pGUIControl)->PythonSetDisabledColor(self->dwDisabledColor);
    }
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef ControlButton_methods[] = {
    {"setLabel", (PyCFunction)ControlButton_SetLabel, METH_KEYWORDS, setLabel__doc__},
    {"setDisabledColor", (PyCFunction)ControlButton_SetDisabledColor, METH_VARARGS, setDisabledColor__doc__},
    {NULL, NULL, 0, NULL}
  };

  // ControlButton class
  PyDoc_STRVAR(controlButton__doc__,
    "ControlButton class.\n"
    "\n"
    "ControlButton(x, y, width, height, label[, focusTexture, noFocusTexture, textXOffset, textYOffset,\n"
    "              alignment, font, textColor, disabledColor, angle, shadowColor, focusedColor])\n"
    "\n"
    "x              : integer - x coordinate of control.\n"
    "y              : integer - y coordinate of control.\n"
    "width          : integer - width of control.\n"
    "height         : integer - height of control.\n"
    "label          : string or unicode - text string.\n"
    "focusTexture   : [opt] string - filename for focus texture.\n"
    "noFocusTexture : [opt] string - filename for no focus texture.\n"
    "textXOffset    : [opt] integer - x offset of label.\n"
    "textYOffset    : [opt] integer - y offset of label.\n"
    "alignment      : [opt] integer - alignment of label - *Note, see xbfont.h\n"
    "font           : [opt] string - font used for label text. (e.g. 'font13')\n"
    "textColor      : [opt] hexstring - color of enabled button's label. (e.g. '0xFFFFFFFF')\n"
    "disabledColor  : [opt] hexstring - color of disabled button's label. (e.g. '0xFFFF3300')\n"
    "angle          : [opt] integer - angle of control. (+ rotates CCW, - rotates CW)\n"
    "shadowColor    : [opt] hexstring - color of button's label's shadow. (e.g. '0xFF000000')\n"
    "focusedColor   : [opt] hexstring - color of focused button's label. (e.g. '0xFF00FFFF')\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "       After you create the control, you need to add it to the window with addControl().\n"
    "\n"
    "example:\n"
    "  - self.button = xbmcgui.ControlButton(100, 250, 200, 50, 'Status', font='font14')\n");

  // Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

  PyTypeObject ControlButton_Type;

  void initControlButton_Type()
  {
    PyInitializeTypeObject(&ControlButton_Type);

    ControlButton_Type.tp_name = "xbmcgui.ControlButton";
    ControlButton_Type.tp_basicsize = sizeof(ControlButton);
    ControlButton_Type.tp_dealloc = (destructor)ControlButton_Dealloc;
    ControlButton_Type.tp_compare = 0;
    ControlButton_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    ControlButton_Type.tp_doc = controlButton__doc__;
    ControlButton_Type.tp_methods = ControlButton_methods;
    ControlButton_Type.tp_base = &Control_Type;
    ControlButton_Type.tp_new = ControlButton_New;
  }
}

#ifdef __cplusplus
}
#endif
