#include "stdafx.h"
#include "..\..\..\settings.h"
#include "..\..\..\applicationmessenger.h"
#include "..\..\..\utils/log.h"
#include "..\python.h"
#include "player.h"
#include "playlist.h"
#include "keyboard.h"

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
/*****************************************************************
 * start of xbmc methods
 *****************************************************************/

	PyObject* XBMC_Output(PyObject *self, PyObject *args)
	{
		char *s_line;
		if (!PyArg_ParseTuple(args, "s:xb_output", &s_line))	return NULL;

		OutputDebugString(s_line);
		CLog::Log(s_line);

		ThreadMessage tMsg = {TMSG_WRITE_SCRIPT_OUTPUT};
		tMsg.strParam = s_line;
		g_applicationMessenger.SendMessage(tMsg);

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* XBMC_Shutdown(PyObject *self, PyObject *args)
	{
		ThreadMessage tMsg = {TMSG_SHUTDOWN};
		g_applicationMessenger.SendMessage(tMsg);

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* XBMC_Dashboard(PyObject *self, PyObject *args)
	{
		ThreadMessage tMsg = {TMSG_DASHBOARD};
		g_applicationMessenger.SendMessage(tMsg);

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* XBMC_Restart(PyObject *self, PyObject *args)
	{
		ThreadMessage tMsg = {TMSG_RESTART};
		g_applicationMessenger.SendMessage(tMsg);

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* XBMC_ExecuteScript(PyObject *self, PyObject *args)
	{
		char *cLine;
		if (!PyArg_ParseTuple(args, "s", &cLine))	return NULL;

		ThreadMessage tMsg = {TMSG_EXECUTE_SCRIPT};
		tMsg.strParam = cLine;
		g_applicationMessenger.SendMessage(tMsg);

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* XBMC_GetSkinDir(PyObject *self, PyObject *args)
	{
		return PyString_FromString(g_stSettings.szDefaultSkin);
	}
	
	// define c functions to be used in python here
	PyMethodDef xbmcMethods[] = {
		{"output", (PyCFunction)XBMC_Output, METH_VARARGS, "output(line) writes a message to debug terminal"},
		{"executescript", (PyCFunction)XBMC_ExecuteScript, METH_VARARGS, ""},
		{"shutdown", (PyCFunction)XBMC_Shutdown, METH_VARARGS, ""},
		{"dashboard", (PyCFunction)XBMC_Dashboard, METH_VARARGS, ""},
		{"restart", (PyCFunction)XBMC_Restart, METH_VARARGS, ""},
		{"getSkinDir", (PyCFunction)XBMC_GetSkinDir, METH_VARARGS, ""},
		{NULL, NULL, 0, NULL}
	};

/*****************************************************************
 * end of methods and python objects
 * initxbmc(void);
 *****************************************************************/

	PyMODINIT_FUNC
	initxbmc(void) 
	{
		// init general xbmc modules
		PyObject* pXbmcModule;

		if (PyType_Ready(&Keyboard_Type) < 0 ||
				PyType_Ready(&Player_Type) < 0 ||
				PyType_Ready(&PlayList_Type) < 0 ||
				PyType_Ready(&PlayListItem_Type)) return;
		
		Py_INCREF(&Keyboard_Type);
		Py_INCREF(&Player_Type);
		Py_INCREF(&PlayList_Type);
		Py_INCREF(&PlayListItem_Type);

		pXbmcModule = Py_InitModule("xbmc", xbmcMethods);
		if (pXbmcModule == NULL) return;

		PyModule_AddObject(pXbmcModule, "Keyboard", (PyObject*)&Keyboard_Type);
		PyModule_AddObject(pXbmcModule, "Player", (PyObject*)&Player_Type);
		PyModule_AddObject(pXbmcModule, "PlayList", (PyObject*)&PlayList_Type);
		PyModule_AddObject(pXbmcModule, "PlayListItem", (PyObject*)&PlayListItem_Type);
	}
}

#ifdef __cplusplus
}
#endif
