#include "../../../stdafx.h"
#include "..\python.h"
#include "player.h"
#include "pyplaylist.h"
#include "keyboard.h"
#include "..\..\..\xbox\iosupport.h"
#include <ConIo.h>
#include "infotagvideo.h"
#include "infotagmusic.h"

// include for constants
#include "pyutil.h"
#include "..\..\..\playlistplayer.h"

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

	PyDoc_STRVAR(output__doc__,
		"output(string) -- Write a string to xbmc's log file and the debug window.\n"
		"\n"
		"Strings are written to the log with log level 1 (NOTICE)");

	PyObject* XBMC_Output(PyObject *self, PyObject *args)
	{
		char *s_line;
		if (!PyArg_ParseTuple(args, "s:xb_output", &s_line))	return NULL;

		CLog::Log(LOGINFO, s_line);

		ThreadMessage tMsg = {TMSG_WRITE_SCRIPT_OUTPUT};
		tMsg.strParam = s_line;
		g_applicationMessenger.SendMessage(tMsg);

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(log__doc__,
		"log(string) -- Write a string to xbmc's log file with log level 6.\n");

	PyObject* XBMC_Log(PyObject *self, PyObject *args)
	{
		char *s_line;
		if (!PyArg_ParseTuple(args, "s", &s_line))	return NULL;

		CLog::Log(LOGFATAL, s_line);

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(shutdown__doc__,
		"shutdown() -- Shutdown the xbox.\n");

	PyObject* XBMC_Shutdown(PyObject *self, PyObject *args)
	{
		ThreadMessage tMsg = {TMSG_SHUTDOWN};
		g_applicationMessenger.SendMessage(tMsg);

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(dashboard__doc__,
		"dashboard() -- Boot to dashboard.\n");

	PyObject* XBMC_Dashboard(PyObject *self, PyObject *args)
	{
		ThreadMessage tMsg = {TMSG_DASHBOARD};
		g_applicationMessenger.SendMessage(tMsg);

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(restart__doc__,
		"restart() -- Restart Xbox.\n");

	PyObject* XBMC_Restart(PyObject *self, PyObject *args)
	{
		ThreadMessage tMsg = {TMSG_RESTART};
		g_applicationMessenger.SendMessage(tMsg);

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(executeScript__doc__,
		"executescript(string) -- Execute a python script.\n"
		"\n"
		"example:\n"
		"  - executescript('q:\\scripts\\update.py')\n");

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

	PyDoc_STRVAR(sleep__doc__,
		"sleep(int time) -- Sleeps for 'time' msec.\n"
		"\n"
		"Throws: PyExc_TypeError, if time is not a  integer\n"
		"This is usefull you have for example a Player class that is waiting for onPlayBackEnded()\n"
		"calls");

	PyObject* XBMC_Sleep(PyObject *self, PyObject *args)
	{
		PyObject *pObject;
		if (!PyArg_ParseTuple(args, "O", &pObject))	return NULL;
		if (!PyInt_Check(pObject))
		{
			PyErr_Format(PyExc_TypeError, "argument must be a bool(integer) value");
			return NULL;
		}

		long i = PyInt_AsLong(pObject);
		while(i != 0)
		{
			Py_BEGIN_ALLOW_THREADS
			Sleep(500);
			Py_END_ALLOW_THREADS

			Py_MakePendingCalls();
			i = PyInt_AsLong(pObject);
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(getLocalizedString__doc__,
		"getLocalizedString(int id) -- Returns a Localized 'unicode string'.\n"
		"\n"
		"See the xml language files in /language/ which id you need for a string\n");

	PyObject* XBMC_GetLocalizedString(PyObject *self, PyObject *args)
	{
		int iString;
		if (!PyArg_ParseTuple(args, "i", &iString))	return NULL;

		return Py_BuildValue("u", g_localizeStrings.Get(iString).c_str());
	}

	PyDoc_STRVAR(getSkinDir__doc__,
		"getSkinDir() -- Returns the active skin directory.\n"
		"\n"
		"Note, this is not the full path like 'q:\\skins\\MediaCenter', \n"
		"but only 'MediaCenter'\n");

	PyObject* XBMC_GetSkinDir(PyObject *self, PyObject *args)
	{
		return PyString_FromString(g_guiSettings.GetString("LookAndFeel.Skin"));
	}

	PyDoc_STRVAR(getLanguage__doc__,
		"getLanguage() -- Returns the active language as string.\n");

	PyObject* XBMC_GetLanguage(PyObject *self, PyObject *args)
	{
		return PyString_FromString(g_guiSettings.GetString("LookAndFeel.Language"));
	}

	PyDoc_STRVAR(getIPAddress__doc__,
		"getIPAddress() -- Returns the current ip adres as string.\n");

	PyObject* XBMC_GetIPAddress(PyObject *self, PyObject *args)
	{
		char cTitleIP[32];
		XNADDR xna;
		XNetGetTitleXnAddr(&xna);
		XNetInAddrToString(xna.ina, cTitleIP, 32);
		return PyString_FromString(cTitleIP);
	}

	PyDoc_STRVAR(getDVDState__doc__,
		"getDVDState() -- Returns the dvd state.\n"
		"\n"
		"return values are:\n"
		"\n"
		"  16 : xbmc.TRAY_OPEN\n"
		"  1  : xbmc.DRIVE_NOT_READY\n"
		"  64 : xbmc.TRAY_CLOSED_NO_MEDIA\n"
		"  96 : xbmc.TRAY_CLOSED_MEDIA_PRESENT");

	PyObject* XBMC_GetDVDState(PyObject *self, PyObject *args)
	{
		CIoSupport io;
		return PyInt_FromLong(io.GetTrayState());
	}

	PyDoc_STRVAR(getFreeMem__doc__,
		"getFreeMem() -- Returns free memory as a string.\n");

	PyObject* XBMC_GetFreeMem(PyObject *self, PyObject *args)
	{
		MEMORYSTATUS stat;
		GlobalMemoryStatus(&stat);
		return PyInt_FromLong( stat.dwAvailPhys  / ( 1024 * 1024 ) );
	}

	PyDoc_STRVAR(getCpuTemp__doc__,
		"getCpuTemp() -- Returns the current cpu tempature.\n");

	PyObject* XBMC_GetCpuTemp(PyObject *self, PyObject *args)
	{
		unsigned short cputemp;
		unsigned short cpudec;

		_outp(0xc004, (0x4c<<1)|0x01);
		_outp(0xc008, 0x01);
		_outpw(0xc000, _inpw(0xc000));
		_outp(0xc002, (0) ? 0x0b : 0x0a);
		while ((_inp(0xc000) & 8));
		cputemp = _inpw(0xc006);
	
		_outp(0xc004, (0x4c<<1)|0x01);
		_outp(0xc008, 0x10);
		_outpw(0xc000, _inpw(0xc000));
		_outp(0xc002, (0) ? 0x0b : 0x0a);
		while ((_inp(0xc000) & 8));
		cpudec = _inpw(0xc006);
	
		if (cpudec<10) cpudec = cpudec * 100;
		if (cpudec<100)	cpudec = cpudec *10; 

		return PyInt_FromLong((long)(cputemp + cpudec / 1000.0f));
	}

	// define c functions to be used in python here
	PyMethodDef xbmcMethods[] = {
		{"output", (PyCFunction)XBMC_Output, METH_VARARGS, output__doc__},
		{"log", (PyCFunction)XBMC_Log, METH_VARARGS, log__doc__},
		{"executescript", (PyCFunction)XBMC_ExecuteScript, METH_VARARGS, executeScript__doc__},
		{"sleep", (PyCFunction)XBMC_Sleep, METH_VARARGS, sleep__doc__},
		{"shutdown", (PyCFunction)XBMC_Shutdown, METH_VARARGS, shutdown__doc__},
		{"dashboard", (PyCFunction)XBMC_Dashboard, METH_VARARGS, dashboard__doc__},
		{"restart", (PyCFunction)XBMC_Restart, METH_VARARGS, restart__doc__},
		{"getSkinDir", (PyCFunction)XBMC_GetSkinDir, METH_VARARGS, getSkinDir__doc__},
		{"getLocalizedString", (PyCFunction)XBMC_GetLocalizedString, METH_VARARGS, getLocalizedString__doc__},

		{"getLanguage", (PyCFunction)XBMC_GetLanguage, METH_VARARGS, getLanguage__doc__},
		{"getIPAddress", (PyCFunction)XBMC_GetIPAddress, METH_VARARGS, getIPAddress__doc__},
		{"getDVDState", (PyCFunction)XBMC_GetDVDState, METH_VARARGS, getDVDState__doc__},
		{"getFreeMem", (PyCFunction)XBMC_GetFreeMem, METH_VARARGS, getFreeMem__doc__},		
		{"getCpuTemp", (PyCFunction)XBMC_GetCpuTemp, METH_VARARGS, getCpuTemp__doc__},

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
				PyType_Ready(&PlayListItem_Type) < 0 ||
				PyType_Ready(&InfoTagMusic_Type) < 0 ||
				PyType_Ready(&InfoTagVideo_Type) < 0) return;
		
		Py_INCREF(&Keyboard_Type);
		Py_INCREF(&Player_Type);
		Py_INCREF(&PlayList_Type);
		Py_INCREF(&PlayListItem_Type);
		Py_INCREF(&InfoTagMusic_Type);
		Py_INCREF(&InfoTagVideo_Type);

		pXbmcModule = Py_InitModule("xbmc", xbmcMethods);
		if (pXbmcModule == NULL) return;

		PyModule_AddObject(pXbmcModule, "Keyboard", (PyObject*)&Keyboard_Type);
		PyModule_AddObject(pXbmcModule, "Player", (PyObject*)&Player_Type);
		PyModule_AddObject(pXbmcModule, "PlayList", (PyObject*)&PlayList_Type);
		PyModule_AddObject(pXbmcModule, "PlayListItem", (PyObject*)&PlayListItem_Type);
		PyModule_AddObject(pXbmcModule, "InfoTagMusic", (PyObject*)&InfoTagMusic_Type);
		PyModule_AddObject(pXbmcModule, "InfoTagVideo", (PyObject*)&InfoTagVideo_Type);

		// constants
		PyModule_AddStringConstant(pXbmcModule, "__author__",			PY_XBMC_AUTHOR);
		PyModule_AddStringConstant(pXbmcModule, "__date__",				"18 August 2004");
		PyModule_AddStringConstant(pXbmcModule, "__version__",		"1.1");
		PyModule_AddStringConstant(pXbmcModule, "__credits__",		PY_XBMC_CREDITS);
		PyModule_AddStringConstant(pXbmcModule, "__platform__",		PY_XBMC_PLATFORM);

		// playlist constants
		PyModule_AddIntConstant(pXbmcModule, "PLAYLIST_MUSIC", PLAYLIST_MUSIC);
		PyModule_AddIntConstant(pXbmcModule, "PLAYLIST_MUSIC_TEMP", PLAYLIST_MUSIC_TEMP);
		PyModule_AddIntConstant(pXbmcModule, "PLAYLIST_VIDEO", PLAYLIST_VIDEO);
		PyModule_AddIntConstant(pXbmcModule, "PLAYLIST_VIDEO_TEMP", PLAYLIST_VIDEO_TEMP);

		// dvd state constants
		PyModule_AddIntConstant(pXbmcModule, "TRAY_OPEN", TRAY_OPEN);
		PyModule_AddIntConstant(pXbmcModule, "DRIVE_NOT_READY", DRIVE_NOT_READY);
		PyModule_AddIntConstant(pXbmcModule, "TRAY_CLOSED_NO_MEDIA", TRAY_CLOSED_NO_MEDIA);
		PyModule_AddIntConstant(pXbmcModule, "TRAY_CLOSED_MEDIA_PRESENT", TRAY_CLOSED_MEDIA_PRESENT);
	}
}

#ifdef __cplusplus
}
#endif
