#include "stdafx.h"
#include "..\python.h"
#include "..\structmember.h"
#include "control.h"
#include "window.h"
#include "dialog.h"
#include "pyutil.h"
#include "guiwindowmanager.h"

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
	PyDoc_STRVAR(lock__doc__,
		"lock() -- Lock the gui until unlock is called.\n"
		"\n"
		"This will improve performance when doing a lot of gui manipulation at once.\n"
		"Note, the main program (xbmc itself) will freeze until unlock is called");

	PyObject* XBMCGUI_Lock(PyObject *self, PyObject *args)
	{
		PyGUILock();
		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(unlock__doc__,
		"unlock() -- Unlock the gui.\n");

	PyObject* XBMCGUI_Unlock(PyObject *self, PyObject *args)
	{
		PyGUIUnlock();
		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(getCurrentWindowId__doc__,
		"getCurrentWindowId() -- returns the id for the current 'active' window.\n");

	PyObject* XBMCGUI_GetCurrentWindowId(PyObject *self, PyObject *args)
	{
		PyGUILock();
		DWORD dwId = m_gWindowManager.GetActiveWindow();
		PyGUIUnlock();
		return Py_BuildValue("l", dwId);
	}

	// define c functions to be used in python here
	PyMethodDef xbmcGuiMethods[] = {
		{"lock", (PyCFunction)XBMCGUI_Lock, METH_VARARGS, lock__doc__},
		{"unlock", (PyCFunction)XBMCGUI_Unlock, METH_VARARGS, unlock__doc__},
		{"getCurrentWindowId", (PyCFunction)XBMCGUI_GetCurrentWindowId, METH_VARARGS, getCurrentWindowId__doc__},
		{NULL, NULL, 0, NULL}
	};

	PyDoc_STRVAR(xbmcgui_module_documentation,
			"XBMC GUI Module"
			"\n"
			"");

	PyMODINIT_FUNC
	initxbmcgui(void) 
	{
		// init xbmc gui modules
		PyObject* pXbmcGuiModule;

		DialogProgress_Type.tp_new = PyType_GenericNew;
		Dialog_Type.tp_new = PyType_GenericNew;

		if (PyType_Ready(&Window_Type) < 0 ||
				PyType_Ready(&WindowDialog_Type) < 0 ||
				PyType_Ready(&ListItem_Type) < 0 ||
				PyType_Ready(&Control_Type) < 0 ||
				PyType_Ready(&ControlSpin_Type) < 0 ||
				PyType_Ready(&ControlLabel_Type) < 0 ||
				PyType_Ready(&ControlFadeLabel_Type) < 0 ||
				PyType_Ready(&ControlTextBox_Type) < 0 ||
				PyType_Ready(&ControlButton_Type) < 0 ||
				PyType_Ready(&ControlList_Type) < 0 ||
				PyType_Ready(&ControlImage_Type) < 0 ||
				PyType_Ready(&Dialog_Type) < 0 ||
				PyType_Ready(&DialogProgress_Type) < 0)
			return;

		Py_INCREF(&Window_Type);
		Py_INCREF(&WindowDialog_Type);
		Py_INCREF(&ListItem_Type);
		Py_INCREF(&Control_Type);
		Py_INCREF(&ControlSpin_Type);
		Py_INCREF(&ControlLabel_Type);
		Py_INCREF(&ControlFadeLabel_Type);
		Py_INCREF(&ControlTextBox_Type);
		Py_INCREF(&ControlButton_Type);
		Py_INCREF(&ControlList_Type);
		Py_INCREF(&ControlImage_Type);
		Py_INCREF(&Dialog_Type);
		Py_INCREF(&DialogProgress_Type);

		pXbmcGuiModule = Py_InitModule3("xbmcgui", xbmcGuiMethods, xbmcgui_module_documentation);

		if (pXbmcGuiModule == NULL) return;

    PyModule_AddObject(pXbmcGuiModule, "Window", (PyObject*)&Window_Type);
		PyModule_AddObject(pXbmcGuiModule, "WindowDialog", (PyObject*)&WindowDialog_Type);
		PyModule_AddObject(pXbmcGuiModule, "ListItem", (PyObject*)&ListItem_Type);
		//PyModule_AddObject(pXbmcGuiModule, "Control", (PyObject*)&Control_Type);
		//PyModule_AddObject(pXbmcGuiModule, "ControlSpin", (PyObject*)&ControlSpin_Type);
		PyModule_AddObject(pXbmcGuiModule, "ControlLabel", (PyObject*)&ControlLabel_Type);
		PyModule_AddObject(pXbmcGuiModule, "ControlFadeLabel", (PyObject*)&ControlFadeLabel_Type);
		PyModule_AddObject(pXbmcGuiModule, "ControlTextBox", (PyObject*)&ControlTextBox_Type);
		PyModule_AddObject(pXbmcGuiModule, "ControlButton", (PyObject*)&ControlButton_Type);
		PyModule_AddObject(pXbmcGuiModule, "ControlList", (PyObject*)&ControlList_Type);
		PyModule_AddObject(pXbmcGuiModule, "ControlImage", (PyObject*)&	ControlImage_Type);
		PyModule_AddObject(pXbmcGuiModule, "Dialog", (PyObject *)&Dialog_Type);
		PyModule_AddObject(pXbmcGuiModule, "DialogProgress", (PyObject *)&DialogProgress_Type);

		// constants
		PyModule_AddStringConstant(pXbmcGuiModule, "__author__",		PY_XBMC_AUTHOR);
		PyModule_AddStringConstant(pXbmcGuiModule, "__date__",			"12 April 2004");
		PyModule_AddStringConstant(pXbmcGuiModule, "__version__",		"1.0");
		PyModule_AddStringConstant(pXbmcGuiModule, "__credits__",		PY_XBMC_CREDITS);
	}
}

#ifdef __cplusplus
}
#endif
