#include "..\python.h"
#include "GUIWindow.h"
#include "control.h"
#include <vector>
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
	typedef struct {
    PyObject_HEAD
		int iWindowId;
		int iOldWindowId;
		int iCurrentControlId;
		bool bIsCreatedByPython;
		CGUIWindow* pWindow;
		std::vector<Control*> vecControls;
	} Window;

	extern PyMethodDef Window_methods[];
	extern PyTypeObject Window_Type;
}

#ifdef __cplusplus
}
#endif
