#include "..\python.h"
#include "GUIPythonWindow.h"
#include "GUIPythonWindowDialog.h"
#include "control.h"

#pragma once

#define Window_Check(op) PyObject_TypeCheck(op, &Window_Type)
#define Window_CheckExact(op) ((op)->ob_type == &Window_Type)

#define WindowDialog_Check(op) PyObject_TypeCheck(op, &WindowDialog_Type)
#define WindowDialog_CheckExact(op) ((op)->ob_type == &WindowDialog_Type)

#define PyObject_HEAD_XBMC_WINDOW		\
    PyObject_HEAD \
		int iWindowId; \
		int iOldWindowId; \
		int iCurrentControlId; \
		bool bIsPythonWindow; \
		bool bModal; \
		CGUIWindow* pWindow; \
		std::vector<Control*> vecControls;

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
	typedef struct {
		PyObject_HEAD_XBMC_WINDOW
	} Window;

	extern PyMethodDef Window_methods[];
	extern PyTypeObject Window_Type;

	bool Window_CreateNewWindow(Window* pWindow, bool bAsDialog);
	void Window_Dealloc(Window* self);
}

#ifdef __cplusplus
}
#endif
