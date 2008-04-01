#include "stdafx.h"
#ifndef _LINUX
#include "../python/Python.h"
#else
#include <python2.4/Python.h>
#include "../XBPythonDll.h"
#endif
#include "GUIRadioButtonControl.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "GUIFontManager.h"
#include "control.h"
#include "pyutil.h"

using namespace std;

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
  PyObject* ControlRadioButton_New(
    PyTypeObject *type,
    PyObject *args,
    PyObject *kwds )
  {
    static char *keywords[] = {
      "x", "y", "width", "height", "label",
      "focusTexture", "noFocusTexture",
      "textXOffset", "textYOffset", "alignment",
      "font", "textColor", "disabledColor", "angle", "shadowColor", "focusedColor",
      "TextureRadioFocus", "TextureRadioNoFocus", NULL };
    ControlRadioButton *self;
    char* cFont = NULL;
    char* cTextureFocus = NULL;
    char* cTextureNoFocus = NULL;
    char* cTextColor = NULL;
    char* cDisabledColor = NULL;
    char* cShadowColor = NULL;
    char* cFocusedColor = NULL;
    char* cTextureRadioFocus = NULL;
    char* cTextureRadioNoFocus = NULL;

    PyObject* pObjectText;

    self = (ControlRadioButton*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    new(&self->strFont) string();    
    new(&self->strText) string();    
    new(&self->strTextureFocus) string();    
    new(&self->strTextureNoFocus) string(); 
    new(&self->strTextureRadioFocus) string();    
    new(&self->strTextureRadioNoFocus) string(); 
    
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
      "llllO|sslllssslssss",
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
      &cFocusedColor,
      &cTextureRadioFocus,
      &cTextureRadioNoFocus))
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
      PyGetDefaultImage("radiobutton", "texturefocus", "radiobutton-focus.png");
    self->strTextureNoFocus = cTextureNoFocus ?
      cTextureNoFocus :
      PyGetDefaultImage("radiobutton", "texturenofocus", "radiobutton-nofocus.jpg");
    self->strTextureRadioFocus = cTextureRadioFocus ?
      cTextureRadioFocus :
      PyGetDefaultImage("radiobutton", "textureradiofocus", "radiobutton-focus.png");
    self->strTextureRadioNoFocus = cTextureRadioNoFocus ?
      cTextureRadioNoFocus :
      PyGetDefaultImage("radiobutton", "textureradionofocus", "radiobutton-nofocus.jpg");

    if (cFont) self->strFont = cFont;
    if (cTextColor) sscanf( cTextColor, "%x", &self->dwTextColor );
    if (cDisabledColor) sscanf( cDisabledColor, "%x", &self->dwDisabledColor );
    if (cShadowColor) sscanf( cShadowColor, "%x", &self->dwShadowColor );
    if (cFocusedColor) sscanf( cFocusedColor, "%x", &self->dwFocusedColor );
    return (PyObject*)self;
  }

  void ControlRadioButton_Dealloc(ControlRadioButton* self)
  {
    self->strFont.~string();
    self->strText.~string();
    self->strTextureFocus.~string();
    self->strTextureNoFocus.~string();
    self->strTextureRadioFocus.~string();
    self->strTextureRadioNoFocus.~string();
    self->ob_type->tp_free((PyObject*)self);
  }

  CGUIControl* ControlRadioButton_Create(ControlRadioButton* pControl)
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
    pControl->pGUIControl = new CGUIRadioButtonControl(
      pControl->iParentId,
      pControl->iControlId,
      (float)pControl->dwPosX,
      (float)pControl->dwPosY,
      (float)pControl->dwWidth,
      (float)pControl->dwHeight,
      (CStdString)pControl->strTextureFocus,
      (CStdString)pControl->strTextureNoFocus,
      label,
      (CStdString)pControl->strTextureRadioFocus,
      (CStdString)pControl->strTextureRadioNoFocus);

    CGUIRadioButtonControl* pGuiButtonControl =
      (CGUIRadioButtonControl*)pControl->pGUIControl;

    pGuiButtonControl->SetLabel(pControl->strText);

    return pControl->pGUIControl;
  }

  // setSelected() Method
  PyDoc_STRVAR(setSelected__doc__,
    "setSelected(selected) -- Sets the radio buttons's selected status.\n"
    "\n"
    "selected            : bool - True=selected (on) / False=not selected (off)\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - self.radiobutton.setSelected(True)\n");

  PyObject* ControlRadioButton_SetSelected(ControlRadioButton *self, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = {
      "selected",
      NULL};

    bool selected = false;
 
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "b",
      keywords,
      &selected))
    {
      return NULL;
    }

    PyGUILock();
    if (self->pGUIControl)
      ((CGUIRadioButtonControl*)self->pGUIControl)->SetSelected(selected);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // isSelected() Method
  PyDoc_STRVAR(isSelected__doc__,
    "isSelected() -- Returns the radio buttons's selected status.\n"
    "\n"
    "example:\n"
    "  - is = self.radiobutton.isSelected()\n");

  PyObject* ControlRadioButton_IsSelected(ControlRadioButton *self, PyObject *args)
  {
    bool isSelected = false;

    PyGUILock();
    if (self->pGUIControl)
      isSelected = ((CGUIRadioButtonControl*)self->pGUIControl)->IsSelected();
    PyGUIUnlock();

    return Py_BuildValue("b", isSelected);
  }

  // setLabel() Method
  PyDoc_STRVAR(setLabel__doc__,
    "setLabel(label[, font, textColor, disabledColor, shadowColor, focusedColor]) -- Set's the radio buttons text attributes.\n"
    "\n"
    "label          : string or unicode - text string.\n"
    "font           : [opt] string - font used for label text. (e.g. 'font13')\n"
    "textColor      : [opt] hexstring - color of enabled radio button's label. (e.g. '0xFFFFFFFF')\n"
    "disabledColor  : [opt] hexstring - color of disabled radio button's label. (e.g. '0xFFFF3300')\n"
    "shadowColor    : [opt] hexstring - color of radio button's label's shadow. (e.g. '0xFF000000')\n"
    "focusedColor   : [opt] hexstring - color of focused radio button's label. (e.g. '0xFFFFFF00')\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - self.radiobutton.setLabel('Status', 'font14', '0xFFFFFFFF', '0xFFFF3300', '0xFF000000')\n");

  PyObject* ControlRadioButton_SetLabel(ControlRadioButton *self, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = {
      "label",
      "font",
      "textColor",
      "disabledColor",
      "shadowColor",
      "focusedColor",
      NULL};
    char *cFont = NULL;
    char *cTextColor = NULL;
    char *cDisabledColor = NULL;
    char *cShadowColor = NULL;
    char *cFocusedColor = NULL;
    PyObject *pObjectText = NULL;
 
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "O|sssss",
      keywords,
      &pObjectText,
      &cFont,
      &cTextColor,
      &cDisabledColor,
      &cShadowColor,
      &cFocusedColor))
    {
      return NULL;
    }

    if (!PyGetUnicodeString(self->strText, pObjectText, 1))
    {
      return NULL;
    }

    if (cFont) self->strFont = cFont;
    if (cTextColor) sscanf(cTextColor, "%x", &self->dwTextColor);
    if (cDisabledColor) sscanf( cDisabledColor, "%x", &self->dwDisabledColor );
    if (cShadowColor) sscanf(cShadowColor, "%x", &self->dwShadowColor);
    if (cFocusedColor) sscanf(cFocusedColor, "%x", &self->dwFocusedColor);

    PyGUILock();
    if (self->pGUIControl)
    {
      ((CGUIRadioButtonControl*)self->pGUIControl)->PythonSetLabel(
        self->strFont, self->strText, self->dwTextColor, self->dwShadowColor, self->dwFocusedColor );
      ((CGUIRadioButtonControl*)self->pGUIControl)->PythonSetDisabledColor(self->dwDisabledColor);
    }
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // setRadioDimension() Method
  PyDoc_STRVAR(setRadioDimension__doc__,
    "setRadioDimension(x, y, width, height) -- Sets the radio buttons's radio texture's position and size.\n"
    "\n"
    "x                   : integer - x coordinate of radio texture.\n"
    "y                   : integer - y coordinate of radio texture.\n"
    "width               : integer - width of radio texture.\n"
    "height              : integer - height of radio texture.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - self.radiobutton.setRadioDimension(x=100, y=5, width=20, height=20)\n");

  PyObject* ControlRadioButton_SetRadioDimension(ControlRadioButton *self, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = {
      "x",
      "y",
      "width",
      "height",
      NULL};
 
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "llll",
      keywords,
      &self->dwPosX,
      &self->dwPosY,
      &self->dwWidth,
      &self->dwHeight))
    {
      return NULL;
    }

    PyGUILock();
    if (self->pGUIControl)
      ((CGUIRadioButtonControl*)self->pGUIControl)->SetRadioDimensions((float)self->dwPosX, (float)self->dwPosY, (float)self->dwWidth, (float)self->dwHeight);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef ControlRadioButton_methods[] = {
    {"setSelected", (PyCFunction)ControlRadioButton_SetSelected, METH_KEYWORDS, setSelected__doc__},
    {"isSelected", (PyCFunction)ControlRadioButton_IsSelected, METH_VARARGS, isSelected__doc__},
    {"setLabel", (PyCFunction)ControlRadioButton_SetLabel, METH_KEYWORDS, setLabel__doc__},
    {"setRadioDimension", (PyCFunction)ControlRadioButton_SetRadioDimension, METH_KEYWORDS, setRadioDimension__doc__},
    {NULL, NULL, 0, NULL}
  };

  // ControlRadioButton class
  PyDoc_STRVAR(ControlRadioButton__doc__,
    "ControlRadioButton class.\n"
    "\n"
    "ControlRadioButton(x, y, width, height, label[, focusTexture, noFocusTexture, textXOffset, textYOffset,\n"
    "              alignment, font, textColor, disabledColor, angle, shadowColor, focusedColor,\n"
    "              radioFocusTexture, noRadioFocusTexture])\n"
    "\n"
    "x                   : integer - x coordinate of control.\n"
    "y                   : integer - y coordinate of control.\n"
    "width               : integer - width of control.\n"
    "height              : integer - height of control.\n"
    "label               : string or unicode - text string.\n"
    "focusTexture        : [opt] string - filename for focus texture.\n"
    "noFocusTexture      : [opt] string - filename for no focus texture.\n"
    "textXOffset         : [opt] integer - x offset of label.\n"
    "textYOffset         : [opt] integer - y offset of label.\n"
    "alignment           : [opt] integer - alignment of label - *Note, see xbfont.h\n"
    "font                : [opt] string - font used for label text. (e.g. 'font13')\n"
    "textColor           : [opt] hexstring - color of enabled radio button's label. (e.g. '0xFFFFFFFF')\n"
    "disabledColor       : [opt] hexstring - color of disabled radio button's label. (e.g. '0xFFFF3300')\n"
    "angle               : [opt] integer - angle of control. (+ rotates CCW, - rotates CW)\n"
    "shadowColor         : [opt] hexstring - color of radio button's label's shadow. (e.g. '0xFF000000')\n"
    "focusedColor        : [opt] hexstring - color of focused radio button's label. (e.g. '0xFF00FFFF')\n"
    "radioFocusTexture   : [opt] string - filename for radio focus texture.\n"
    "noRadioFocusTexture : [opt] string - filename for radio no focus texture.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "       After you create the control, you need to add it to the window with addControl().\n"
    "\n"
    "example:\n"
    "  - self.radiobutton = xbmcgui.ControlRadioButton(100, 250, 200, 50, 'Status', font='font14')\n");

  // Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject ControlRadioButton_Type;

  void initControlRadioButton_Type()
  {
    PyInitializeTypeObject(&ControlRadioButton_Type);

    ControlRadioButton_Type.tp_name = "xbmcgui.ControlRadioButton";
    ControlRadioButton_Type.tp_basicsize = sizeof(ControlRadioButton);
    ControlRadioButton_Type.tp_dealloc = (destructor)ControlRadioButton_Dealloc;
    ControlRadioButton_Type.tp_compare = 0;
    ControlRadioButton_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    ControlRadioButton_Type.tp_doc = ControlRadioButton__doc__;
    ControlRadioButton_Type.tp_methods = ControlRadioButton_methods;
    ControlRadioButton_Type.tp_base = &Control_Type;
    ControlRadioButton_Type.tp_new = ControlRadioButton_New;
  }
}

#ifdef __cplusplus
}
#endif
