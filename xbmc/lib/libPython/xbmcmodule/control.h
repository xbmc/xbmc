#include "..\python.h"
#include "GUIControl.h"
#include <vector>
#include "listitem.h"

#pragma once

// python type checking
#define Control_Check(op) PyObject_TypeCheck(op, &Control_Type)
#define Control_CheckExact(op) ((op)->ob_type == &Control_Type)

#define ControlButton_CheckExact(op) ((op)->ob_type == &ControlButton_Type)
#define ControlList_CheckExact(op) ((op)->ob_type == &ControlList_Type)
#define Control_CheckExact(op) ((op)->ob_type == &Control_Type)
#define ControlSpin_CheckExact(op) ((op)->ob_type == &ControlSpin_Type)
#define ControlLabel_CheckExact(op) ((op)->ob_type == &ControlLabel_Type)
#define ControlFadeLabel_CheckExact(op) ((op)->ob_type == &ControlFadeLabel_Type)
#define ControlTextBox_CheckExact(op) ((op)->ob_type == &ControlTextBox_Type)
#define ControlImage_CheckExact(op) ((op)->ob_type == &ControlImage_Type)

#define PyObject_HEAD_XBMC_CONTROL		\
    PyObject_HEAD				\
		int iControlId;			\
		int iParentId;			\
		int dwPosX;					\
		int dwPosY;					\
		int dwWidth;				\
		int dwHeight;				\
		int iControlUp;			\
		int iControlDown;		\
		int iControlLeft;		\
		int iControlRight;	\
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
	extern PyTypeObject ControlList_Type;
}

#ifdef __cplusplus
}
#endif
