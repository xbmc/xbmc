#include "..\python.h"
#include "GUIControl.h"
#pragma once

#define PyObject_HEAD_XBMC_CONTROL		\
    PyObject_HEAD				\
		int iControlId;			\
		int iParentId;			\
		int dwPosX;					\
		int dwPosY;					\
		int dwWidth;				\
		int dwHeight;				\
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
		string strFont;
		wstring strText;
		DWORD dwTextColor;
	} ControlLabel;

	typedef struct {
    PyObject_HEAD_XBMC_CONTROL
		string strFileName;
		DWORD strColorKey;
	} ControlImage;

	extern void Control_Dealloc(Control* self);

	extern PyMethodDef Control_methods[];

	extern PyTypeObject Control_Type;
	extern PyTypeObject ControlLabel_Type;
	extern PyTypeObject ControlImage_Type;
}

#ifdef __cplusplus
}
#endif
