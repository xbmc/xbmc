#include "..\python.h"
#include "GUIControl.h"
#include <vector>

#pragma once

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
		std::vector<string> vecItems;
		string strFont;

		DWORD dwSpinWidth;
		DWORD dwSpinHeight;

    string strTextureUp;
		string strTextureDown;
		string strTextureUpFocus;
		string strTextureDownFocus;

		DWORD dwSpinColor;
		DWORD dwSpinX;
		DWORD dwSpinY;

		DWORD dwTextColor;
		DWORD dwSelectedColor;
		string strButton;
		string strButtonFocus;
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
