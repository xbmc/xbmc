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

#include "lib/libPython/Python/Include/Python.h"
#include "../XBPythonDll.h"
#include "GUIControl.h"
#include "listitem.h"

#pragma once

// python type checking
#define Control_Check(op) PyObject_TypeCheck(op, &Control_Type)
#define Control_CheckExact(op) ((op)->ob_type == &Control_Type)

#define ControlButton_Check(op) PyObject_TypeCheck(op, &ControlButton_Type)
#define ControlButton_CheckExact(op) ((op)->ob_type == &ControlButton_Type)

#define ControlCheckMark_Check(op) PyObject_TypeCheck(op, &ControlCheckMark_Type)
#define ControlCheckMark_CheckExact(op) ((op)->ob_type == &ControlCheckMark_Type)

#define ControlProgress_Check(op) PyObject_TypeCheck(op, &ControlProgress_Type)
#define ControlProgress__CheckExact(op) ((op)->ob_type == &ControlProgress_Type)

#define ControlList_Check(op) PyObject_TypeCheck(op, &ControlList_Type)
#define ControlList_CheckExact(op) ((op)->ob_type == &ControlList_Type)

#define ControlSpin_Check(op) PyObject_TypeCheck(op, &ControlSpin_Type)
#define ControlSpin_CheckExact(op) ((op)->ob_type == &ControlSpin_Type)

#define ControlLabel_Check(op) PyObject_TypeCheck(op, &ControlLabel_Type)
#define ControlLabel_CheckExact(op) ((op)->ob_type == &ControlLabel_Type)

#define ControlFadeLabel_Check(op) PyObject_TypeCheck(op, &ControlFadeLabel_Type)
#define ControlFadeLabel_CheckExact(op) ((op)->ob_type == &ControlFadeLabel_Type)

#define ControlTextBox_Check(op) PyObject_TypeCheck(op, &ControlTextBox_Type)
#define ControlTextBox_CheckExact(op) ((op)->ob_type == &ControlTextBox_Type)

#define ControlImage_Check(op) PyObject_TypeCheck(op, &ControlImage_Type)
#define ControlImage_CheckExact(op) ((op)->ob_type == &ControlImage_Type)

#define ControlGroup_Check(op) PyObject_TypeCheck(op, &ControlGroup_Type)
#define ControlGroup_CheckExact(op) ((op)->ob_type == &ControlGroup_Type)

#define ControlRadioButton_Check(op) PyObject_TypeCheck(op, &ControlRadioButton_Type)
#define ControlRadioButton_CheckExact(op) ((op)->ob_type == &ControlRadioButton_Type)

// -----------------

// hardcoded offsets for button controls (and controls that use button controls)
// ideally they should be dynamically read in as with all the other properties.
#define CONTROL_TEXT_OFFSET_X 10
#define CONTROL_TEXT_OFFSET_Y 2

#define PyObject_HEAD_XBMC_CONTROL		\
    PyObject_HEAD				\
    int iControlId;			\
    int iParentId;			\
    int dwPosX;					\
    int dwPosY;					\
    int dwWidth;				\
    int dwHeight;				\
    DWORD iControlUp;			\
    DWORD iControlDown;		\
    DWORD iControlLeft;		\
    DWORD iControlRight;	\
    CGUIControl* pGUIControl;

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  typedef struct {
    PyObject_HEAD_XBMC_CONTROL
  } Control;

  typedef struct {
    PyObject_HEAD_XBMC_CONTROL
    DWORD dwColor;
    std::string strTextureUp;
    std::string strTextureDown;
    std::string strTextureUpFocus;
    std::string strTextureDownFocus;
  } ControlSpin;

  typedef struct {
    PyObject_HEAD_XBMC_CONTROL
    std::string strFont;
    std::string strText;
    DWORD dwTextColor;
    DWORD dwDisabledColor;
    DWORD dwAlign;
    bool bHasPath;
    int iAngle;
  } ControlLabel;


  typedef struct {
    PyObject_HEAD_XBMC_CONTROL
    std::string strFont;
    DWORD dwTextColor;
    std::vector<std::string> vecLabels;
    DWORD dwAlign;
  } ControlFadeLabel;

  typedef struct {
    PyObject_HEAD_XBMC_CONTROL
    std::string strFont;
    DWORD dwTextColor;
  } ControlTextBox;

  typedef struct {
    PyObject_HEAD_XBMC_CONTROL
    std::string strFileName;
    DWORD aspectRatio;
    DWORD strColorDiffuse;
  } ControlImage;

  typedef struct {
  PyObject_HEAD_XBMC_CONTROL
    std::string strTextureLeft;
    std::string strTextureMid;
    std::string strTextureRight;
    std::string strTextureBg;
    std::string strTextureOverlay;
    DWORD strColorKey;
    DWORD aspectRatio;
    DWORD strColorDiffuse;
  } ControlProgress;

  typedef struct {
    PyObject_HEAD_XBMC_CONTROL
    std::string strFont;
    std::string strText;
    std::string strText2;
    std::string strTextureFocus;
    std::string strTextureNoFocus;
    DWORD dwTextColor;
    DWORD dwDisabledColor;
    DWORD dwTextXOffset;
    DWORD dwTextYOffset;
    DWORD dwAlign;
    int iAngle;
    DWORD dwShadowColor;
    DWORD dwFocusedColor;
  } ControlButton;

  typedef struct {
    PyObject_HEAD_XBMC_CONTROL
    std::string strFont;
    std::string strText;
    std::string strTextureFocus;
    std::string strTextureNoFocus;
    DWORD dwTextColor;
    DWORD dwDisabledColor;
    DWORD dwCheckWidth;
    DWORD dwCheckHeight;
    DWORD dwAlign;
  } ControlCheckMark;

  typedef struct {
    PyObject_HEAD_XBMC_CONTROL
    std::vector<PYXBMC::ListItem*> vecItems;
    std::string strFont;
    ControlSpin* pControlSpin;

    DWORD dwTextColor;
    DWORD dwSelectedColor;
    std::string strTextureButton;
    std::string strTextureButtonFocus;

    DWORD dwImageHeight;
    DWORD dwImageWidth;
    DWORD dwItemHeight;
    DWORD dwSpace;

    int dwItemTextXOffset;
    int dwItemTextYOffset;
    DWORD dwAlignmentY;
  } ControlList;

  typedef struct {
    PyObject_HEAD_XBMC_CONTROL
  } ControlGroup;

  typedef struct {
    PyObject_HEAD_XBMC_CONTROL
    std::string strFont;
    std::string strText;
    std::string strTextureFocus;
    std::string strTextureNoFocus;
    std::string strTextureRadioFocus;
    std::string strTextureRadioNoFocus;
    DWORD dwTextColor;
    DWORD dwDisabledColor;
    DWORD dwTextXOffset;
    DWORD dwTextYOffset;
    DWORD dwAlign;
    int iAngle;
    DWORD dwShadowColor;
    DWORD dwFocusedColor;
  } ControlRadioButton;

  extern void Control_Dealloc(Control* self);

  extern PyMethodDef Control_methods[];

  extern PyTypeObject Control_Type;
  extern PyTypeObject ControlSpin_Type;
  extern PyTypeObject ControlLabel_Type;
  extern PyTypeObject ControlFadeLabel_Type;
  extern PyTypeObject ControlTextBox_Type;
  extern PyTypeObject ControlImage_Type;
  extern PyTypeObject ControlGroup_Type;
  extern PyTypeObject ControlButton_Type;
  extern PyTypeObject ControlCheckMark_Type;
  extern PyTypeObject ControlList_Type;
  extern PyTypeObject ControlProgress_Type;
  extern PyTypeObject ControlRadioButton_Type;

  CGUIControl* ControlLabel_Create(ControlLabel* pControl);
  CGUIControl* ControlFadeLabel_Create(ControlFadeLabel* pControl);
  CGUIControl* ControlTextBox_Create(ControlTextBox* pControl);
  CGUIControl* ControlButton_Create(ControlButton* pControl);
  CGUIControl* ControlCheckMark_Create(ControlCheckMark* pControl);
  CGUIControl* ControlImage_Create(ControlImage* pControl);
  CGUIControl* ControlGroup_Create(ControlGroup* pControl);
  CGUIControl* ControlList_Create(ControlList* pControl);
  CGUIControl* ControlProgress_Create(ControlProgress* pControl);
  CGUIControl* ControlRadioButton_Create(ControlRadioButton* pControl);

  void initControl_Type();
  void initControlSpin_Type();
  void initControlLabel_Type();
  void initControlFadeLabel_Type();
  void initControlTextBox_Type();
  void initControlButton_Type();
  void initControlCheckMark_Type();
  void initControlList_Type();
  void initControlImage_Type();
  void initControlGroup_Type();
  void initControlProgress_Type();
  void initControlRadioButton_Type();
}

#ifdef __cplusplus
}
#endif
