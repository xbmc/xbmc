#include "stdafx.h"
#include "..\..\..\application.h"
#include "..\python.h"
#include "..\structmember.h"
#include "control.h"
#include "window.h"

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
	extern PyTypeObject DialogType;
	extern PyTypeObject DialogProgressType;

	// define c functions to be used in python here
	PyMethodDef xbmcGuiMethods[] = {
		{NULL, NULL, 0, NULL}
	};

	PyMODINIT_FUNC
	initxbmcgui(void) 
	{
		// init xbmc gui modules
		PyObject* pXbmcGuiModule;

		DialogType.tp_new = PyType_GenericNew;
		DialogProgressType.tp_new = PyType_GenericNew;

		if (PyType_Ready(&Window_Type) < 0 ||
				PyType_Ready(&Control_Type) < 0 ||
				PyType_Ready(&ControlLabel_Type) < 0 ||
				PyType_Ready(&ControlFadeLabel_Type) < 0 ||
				PyType_Ready(&ControlButton_Type) < 0 ||
				PyType_Ready(&ControlImage_Type) < 0 ||
				PyType_Ready(&DialogType) < 0 ||
				PyType_Ready(&DialogProgressType) < 0)
			return;

		Py_INCREF(&Window_Type);
		Py_INCREF(&Control_Type);
		Py_INCREF(&ControlLabel_Type);
		Py_INCREF(&ControlFadeLabel_Type);
		Py_INCREF(&ControlButton_Type);
		Py_INCREF(&ControlImage_Type);
		Py_INCREF(&DialogType);
		Py_INCREF(&DialogProgressType);

		pXbmcGuiModule = Py_InitModule("xbmcgui", xbmcGuiMethods);
		if (pXbmcGuiModule == NULL) return;

    PyModule_AddObject(pXbmcGuiModule, "Window", (PyObject*)&Window_Type);
		//PyModule_AddObject(pXbmcGuiModule, "Control", (PyObject*)&Control_Type);
		PyModule_AddObject(pXbmcGuiModule, "ControlLabel", (PyObject*)&ControlLabel_Type);
		PyModule_AddObject(pXbmcGuiModule, "ControlFadeLabel", (PyObject*)&ControlFadeLabel_Type);
		PyModule_AddObject(pXbmcGuiModule, "ControlButton", (PyObject*)&ControlButton_Type);
		PyModule_AddObject(pXbmcGuiModule, "ControlImage", (PyObject*)&	ControlImage_Type);
		PyModule_AddObject(pXbmcGuiModule, "Dialog", (PyObject*)&DialogType);
		PyModule_AddObject(pXbmcGuiModule, "DialogProgress", (PyObject *)&DialogProgressType);
	}
}

#ifdef __cplusplus
}
#endif
