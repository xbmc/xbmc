#include "..\python.h"
#include "..\..\..\application.h"

#define ACTIVE_WINDOW	m_gWindowManager.GetActiveWindow()

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

/*****************************************************************
 * start of dialog methods and python objects
 *****************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
	typedef struct {
    PyObject_HEAD
	} Dialog;

	PyObject* Dialog_OK(PyObject *self, PyObject *args)
	{
		const DWORD dWindow = 2002;
		char *cLine[4];
		CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(dWindow);

		for (int i = 0; i < 4; i++)	cLine[i] = NULL;
		// get lines, last 2 lines are optional.
		if (!PyArg_ParseTuple(args, "ss|ss", &cLine[0], &cLine[1], &cLine[2], &cLine[3]))	return NULL;

		pDialog->SetHeading(cLine[0] ? cLine[0] : "");
		pDialog->SetLine(0, cLine[1] ? cLine[1] : "");
		pDialog->SetLine(1, cLine[2] ? cLine[2] : "");
		pDialog->SetLine(2, cLine[3] ? cLine[3] : "");

		//send message and wait for user input
		ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, dWindow, ACTIVE_WINDOW};
		g_applicationMessenger.SendMessage(tMsg, true);

		return Py_BuildValue("b", pDialog->IsConfirmed());
	}

	PyObject* Dialog_YesNo(PyObject *self, PyObject *args)
	{
		const DWORD dWindow = 100;
		char *cLine[4];
		CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(dWindow);

		for (int i = 0; i < 4; i++)	cLine[i] = NULL;
		// get lines, last 2 lines are optional.
		if (!PyArg_ParseTuple(args, "ss|ss", &cLine[0], &cLine[1], &cLine[2], &cLine[3]))	return NULL;

		pDialog->SetHeading(cLine[0] ? cLine[0] : "");
		pDialog->SetLine(0, cLine[1] ? cLine[1] : "");
		pDialog->SetLine(1, cLine[2] ? cLine[2] : "");
		pDialog->SetLine(2, cLine[3] ? cLine[3] : "");

		//send message and wait for user input
		ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, dWindow, ACTIVE_WINDOW};		
		g_applicationMessenger.SendMessage(tMsg, true);

		return Py_BuildValue("b", pDialog->IsConfirmed());
	}

	PyObject* Dialog_Select(PyObject *self, PyObject *args)
	{
		const DWORD dWindow = 2000;
		char *cHeader;
		PyObject *list = NULL;
			
		if (!PyArg_ParseTuple(args, "sO", &cHeader, &list))	return NULL;
		if (!PyList_Check(list)) return NULL;

		CGUIDialogSelect* pDialog= (CGUIDialogSelect*)m_gWindowManager.GetWindow(dWindow);
		pDialog->Reset();
		pDialog->SetHeading(cHeader);

		PyObject *listLine = NULL;
		for(int i = 0; i < PyList_Size(list); i++)
		{
			listLine = PyList_GetItem(list, i);
			if (PyString_Check(listLine)) pDialog->Add(PyString_AsString(listLine));
		}

		//send message and wait for user input
		ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, dWindow, ACTIVE_WINDOW};
		g_applicationMessenger.SendMessage(tMsg, true);

		return Py_BuildValue("i", pDialog->GetSelectedLabel());
	}

	/* xbmc Dialog functions for use in python */
	PyMethodDef Dialog_methods[] = {
		{"yesno", (PyCFunction)Dialog_YesNo, METH_VARARGS, ""},
		{"select", (PyCFunction)Dialog_Select, METH_VARARGS, ""},
		{"ok", (PyCFunction)Dialog_OK, METH_VARARGS, ""},
		{NULL, NULL, 0, NULL}
	};

	PyTypeObject DialogType = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmcgui.Dialog",             /*tp_name*/
			sizeof(Dialog),            /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			0,                         /*tp_dealloc*/
			0,                         /*tp_print*/
			0,                         /*tp_getattr*/
			0,                         /*tp_setattr*/
			0,                         /*tp_compare*/
			0,                         /*tp_repr*/
			0,                         /*tp_as_number*/
			0,                         /*tp_as_sequence*/
			0,                         /*tp_as_mapping*/
			0,                         /*tp_hash */
			0,                         /*tp_call*/
			0,                         /*tp_str*/
			0,                         /*tp_getattro*/
			0,                         /*tp_setattro*/
			0,                         /*tp_as_buffer*/
			Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
			"Dialog Objects",          /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			Dialog_methods,            /* tp_methods */
			0,                         /* tp_members */
	};

/*****************************************************************
 * start of dialog process methods and python objects
 *****************************************************************/

	typedef struct {
    PyObject_HEAD
	} DialogProgress;

	PyObject* Dialog_ProgressCreate(PyObject *self, PyObject *args)
	{
		const char *cLine[4];
		wchar_t line[128];

		for (int i = 0; i < 4; i++)	cLine[i] = NULL;
		// get lines, last 3 lines are optional.
		if (!PyArg_ParseTuple(args, "s|sss", &cLine[0], &cLine[1], &cLine[2], &cLine[3]))	return NULL;

		CGUIDialogProgress* pDialog= (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

		// convert char strings to wchar strings and set the header + line 1, 2 and 3 for dialog
		mbsrtowcs(line, &cLine[0], 128, NULL);

		g_graphicsContext.Lock();
		pDialog->SetHeading(line);

		for (int i = 0; i < 3; i++)
		{
			if (cLine[i])
				pDialog->SetLine(i,cLine[i]);
			else
				pDialog->SetLine(i,"");
		}

		pDialog->StartModal(m_gWindowManager.GetActiveWindow());
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Dialog_ProgressUpdate(PyObject *self, PyObject *args)
	{
		int percentage = 0;
		if (!PyArg_ParseTuple(args, "i", &percentage))	return NULL;

		CGUIDialogProgress* pDialog= (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

		g_graphicsContext.Lock();
		if (percentage >= 0 && percentage <= 100)
		{
			pDialog->SetPercentage(percentage);
			pDialog->ShowProgressBar(true);
		}
		else
		{
			pDialog->ShowProgressBar(false);
		}
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Dialog_ProgressIsCanceled(PyObject *self, PyObject *args)
	{
		bool canceled = false;
		CGUIDialogProgress* pDialog= (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

		g_graphicsContext.Lock();
		canceled = pDialog->IsCanceled();
		g_graphicsContext.Unlock();

		return Py_BuildValue("b", canceled);
	}

	PyObject* Dialog_ProgressClose(PyObject *self, PyObject *args)
	{
		CGUIDialogProgress* pDialog= (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

		g_graphicsContext.Lock();
		pDialog->Close();
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	/* xbmc progress Dialog functions for use in python */
	static PyMethodDef DialogProgress_methods[] = {
		{"create", (PyCFunction)Dialog_ProgressCreate, METH_VARARGS, ""},
		{"update", (PyCFunction)Dialog_ProgressUpdate, METH_VARARGS, ""},
		{"close", (PyCFunction)Dialog_ProgressClose, METH_VARARGS, ""},
		{"iscanceled", (PyCFunction)Dialog_ProgressIsCanceled, METH_VARARGS, ""},
		{NULL, NULL, 0, NULL}
	};

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject DialogProgressType = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmcgui.DialogProgress",     /*tp_name*/
			sizeof(DialogProgress),    /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			0,                         /*tp_dealloc*/
			0,                         /*tp_print*/
			0,                         /*tp_getattr*/
			0,                         /*tp_setattr*/
			0,                         /*tp_compare*/
			0,                         /*tp_repr*/
			0,                         /*tp_as_number*/
			0,                         /*tp_as_sequence*/
			0,                         /*tp_as_mapping*/
			0,                         /*tp_hash */
			0,                         /*tp_call*/
			0,                         /*tp_str*/
			0,                         /*tp_getattro*/
			0,                         /*tp_setattro*/
			0,                         /*tp_as_buffer*/
			Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
			"DialogProgress Objects",  /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			DialogProgress_methods,    /* tp_methods */
			0,                         /* tp_members */
	};
}

#ifdef __cplusplus
}
#endif
