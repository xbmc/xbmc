#include "..\python.h"
#include "GUIControl.h"
#include <vector>
#include "listitem.h"

#pragma once

// python type checking
#define Control_Check(op) PyObject_TypeCheck(op, &Control_Type)
#define Control_CheckExact(op) ((op)->ob_type == &Control_Type)

#define ControlButton_Check(op) PyObject_TypeCheck(op, &ControlButton_Type)
#define ControlButton_CheckExact(op) ((op)->ob_type == &ControlButton_Type)

#define ControlCheckMark_Check(op) PyObject_TypeCheck(op, &ControlCheckMark_Type)
#define ControlCheckMark_CheckExact(op) ((op)->ob_type == &ControlCheckMark_Type)

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
    string strTextureUp;
		string strTextureDown;
		string strTextureUpFocus;
		string strTextureDownFocus;
	} ControlSpin;

	typedef struct {
    PyObject_HEAD_XBMC_CONTROL
		string strFont;
		wstring strText;
		DWORD dwTextColor;
	} ControlLabel;

	typedef struct {
    PyObject_HEAD_XBMC_CONTROL
		string strFont;
		DWORD dwTextColor;
		std::vector<string> vecLabels;
	} ControlFadeLabel;

	typedef struct {
    PyObject_HEAD_XBMC_CONTROL
		string strFont;
		DWORD dwTextColor;
		ControlSpin* pControlSpin;	
	} ControlTextBox;

	typedef struct {
    PyObject_HEAD_XBMC_CONTROL
		string strFileName;
		DWORD strColorKey;
	} ControlImage;

	typedef struct {
    PyObject_HEAD_XBMC_CONTROL
		string strFont;
		wstring strText;
		string strTextureFocus;
		string strTextureNoFocus;
		DWORD dwTextColor;
		DWORD dwDisabledColor;
	} ControlButton;

	typedef struct {
    PyObject_HEAD_XBMC_CONTROL
		string strFont;
		wstring strText;
		string strTextureFocus;
		string strTextureNoFocus;
		DWORD dwTextColor;
		DWORD dwDisabledColor;
        DWORD dwCheckWidth;
        DWORD dwCheckHeight;
        DWORD dwAlign;
	} ControlCheckMark;

	typedef struct {
    PyObject_HEAD_XBMC_CONTROL
		std::vector<PYXBMC::ListItem*> vecItems;
		string strFont;
		ControlSpin* pControlSpin;

		DWORD dwTextColor;
		DWORD dwSelectedColor;
		string strTextureButton;
		string strTextureButtonFocus;

		DWORD dwImageHeight;
		DWORD dwImageWidth;
		DWORD dwItemHeight;
		DWORD dwSpace;
	} ControlList;

	extern void Control_Dealloc(Control* self);

	extern PyMethodDef Control_methods[];

	extern PyTypeObject Control_Type;
	extern PyTypeObject ControlSpin_Type;
	extern PyTypeObject ControlLabel_Type;
	extern PyTypeObject ControlFadeLabel_Type;
	extern PyTypeObject ControlTextBox_Type;
	extern PyTypeObject ControlImage_Type;
	extern PyTypeObject ControlButton_Type;
	extern PyTypeObject ControlCheckMark_Type;
	extern PyTypeObject ControlList_Type;

	CGUIControl* ControlLabel_Create(ControlLabel* pControl);
	CGUIControl* ControlFadeLabel_Create(ControlFadeLabel* pControl);
	CGUIControl* ControlTextBox_Create(ControlTextBox* pControl);
	CGUIControl* ControlButton_Create(ControlButton* pControl);
	CGUIControl* ControlCheckMark_Create(ControlCheckMark* pControl);
	CGUIControl* ControlImage_Create(ControlImage* pControl);
	CGUIControl* ControlList_Create(ControlList* pControl);
}

#ifdef __cplusplus
}
#endif
