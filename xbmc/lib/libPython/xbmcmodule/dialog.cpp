#include "../../../stdafx.h"
#include "dialog.h"
#include "..\python.h"
#include "pyutil.h"
#include "..\..\..\application.h"

#define ACTIVE_WINDOW	m_gWindowManager.GetActiveWindow()

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
	PyObject* WindowDialog_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		WindowDialog *self;

		self = (WindowDialog*)type->tp_alloc(type, 0);
		if (!self) return NULL;

		self->iWindowId = -1;

		if (!PyArg_ParseTuple(args, "|i", &self->iWindowId)) return NULL;

		// create new GUIWindow
		if (!Window_CreateNewWindow((Window*)self, true))
		{
			// error is already set by Window_CreateNewWindow, just release the memory
			self->ob_type->tp_free((PyObject*)self);
			return NULL;
		}

		return (PyObject*)self;
	}

	PyDoc_STRVAR(ok__doc__,
		"ok(heading, line 1[, line 2, line3]) -- Show a dialog 'OK'.\n"
		"\n"
		"heading     : string or unicode string\n"
		"lines       : string or unicode string\n"
		"returns True when user pressed 'OK'");

	PyObject* Dialog_OK(PyObject *self, PyObject *args)
	{
		const DWORD dWindow = WINDOW_DIALOG_OK;
		PyObject* unicodeLine[4];
    for (int i = 0; i < 4; i++) unicodeLine[i] = NULL;

    CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(dWindow);
		if (PyWindowIsNull(pDialog)) return NULL;

		// get lines, last 2 lines are optional.
    string utf8Line[4];
		if (!PyArg_ParseTuple(args, "OO|OO", &unicodeLine[0], &unicodeLine[1], &unicodeLine[2], &unicodeLine[3]))	return NULL;

    for (int i = 0; i < 4; i++)
    {
      if (unicodeLine[i] && !PyGetUnicodeString(utf8Line[i], unicodeLine[i], i+1))
        return NULL;
    }
		pDialog->SetHeading(utf8Line[0]);
		pDialog->SetLine(0, utf8Line[1]);
		pDialog->SetLine(1, utf8Line[2]);
		pDialog->SetLine(2, utf8Line[3]);

		//send message and wait for user input
		ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, dWindow, ACTIVE_WINDOW};
		g_applicationMessenger.SendMessage(tMsg, true);

		return Py_BuildValue("b", pDialog->IsConfirmed());
	}

	PyDoc_STRVAR(yesno__doc__,
		"yesno(heading, line 1[, line 2, line3]) -- Show a dialog 'YES / NO'.\n"
		"\n"
		"heading     : string or unicode string\n"
		"lines       : string or unicode string\n"
		"returns True when user pressed 'OK', else False");

	PyObject* Dialog_YesNo(PyObject *self, PyObject *args)
	{
		const DWORD dWindow = WINDOW_DIALOG_YES_NO;
		PyObject* unicodeLine[4];
    for (int i = 0; i < 4; i++) unicodeLine[i] = NULL;
		CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(dWindow);
		if (PyWindowIsNull(pDialog)) return NULL;

		// get lines, last 2 lines are optional.
    string utf8Line[4];
		if (!PyArg_ParseTuple(args, "OO|OO", &unicodeLine[0], &unicodeLine[1], &unicodeLine[2], &unicodeLine[3]))	return NULL;

    for (int i = 0; i < 4; i++)
    {
      if (unicodeLine[i] && !PyGetUnicodeString(utf8Line[i], unicodeLine[i], i+1))
        return NULL;
    }
		pDialog->SetHeading(utf8Line[0]);
		pDialog->SetLine(0, utf8Line[1]);
		pDialog->SetLine(1, utf8Line[2]);
		pDialog->SetLine(2, utf8Line[3]);

		//send message and wait for user input
		ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, dWindow, ACTIVE_WINDOW};		
		g_applicationMessenger.SendMessage(tMsg, true);

		return Py_BuildValue("b", pDialog->IsConfirmed());
	}

	PyDoc_STRVAR(select__doc__,
		"select(heading, list) -- Show a select dialog.\n"
		"\n"
		"heading     : string or unicode string\n"
		"list        : python list with strings\n"
		"returns the position of the selected item");

	PyObject* Dialog_Select(PyObject *self, PyObject *args)
	{
		const DWORD dWindow = WINDOW_DIALOG_SELECT;
		char *cHeader;
		PyObject *list = NULL;
			
		if (!PyArg_ParseTuple(args, "sO", &cHeader, &list))	return NULL;
		if (!PyList_Check(list)) return NULL;

		CGUIDialogSelect* pDialog= (CGUIDialogSelect*)m_gWindowManager.GetWindow(dWindow);
		if (PyWindowIsNull(pDialog)) return NULL;

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

/*****************************************************************
 * start of dialog process methods and python objects
 *****************************************************************/

	PyDoc_STRVAR(create__doc__,
		"create(heading[, line 1, line 2, line3]) -- Create and show a progress dialog.\n"
		"\n"
		"heading     : string or unicode string\n"
		"lines       : string or unicode string\n");

	PyObject* Dialog_ProgressCreate(PyObject *self, PyObject *args)
	{
		PyObject* unicodeLine[4];
    for (int i = 0; i < 4; i++) unicodeLine[i] = NULL;

		// get lines, last 3 lines are optional.
		if (!PyArg_ParseTuple(args, "O|OOO", &unicodeLine[0], &unicodeLine[1], &unicodeLine[2], &unicodeLine[3]))	return NULL;

    string utf8Line[4];
    for (int i = 0; i < 4; i++)
    {
      if (unicodeLine[i] && !PyGetUnicodeString(utf8Line[i], unicodeLine[i], i+1))
        return NULL;
    }

		CGUIDialogProgress* pDialog= (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
		if (PyWindowIsNull(pDialog)) return NULL;

		g_graphicsContext.Lock();
		pDialog->SetHeading(utf8Line[0]);

		for (int i = 1; i < 4; i++)
			pDialog->SetLine(i,utf8Line[i]);

		pDialog->StartModal(m_gWindowManager.GetActiveWindow());
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(update__doc__,
		"update(int value[, line 1, line 2, line3]) -- Update's the progress dialog.\n"
		"\n"
		"value       : int from 0 - 100\n"
		"lines       : string or unicode string\n"
		"\n"
		"If value is 0, the progress bar will be hidden.");

	PyObject* Dialog_ProgressUpdate(PyObject *self, PyObject *args)
	{
		int percentage = 0;
		PyObject *unicodeLine[3];
		for (int i = 0; i < 3; i++)	unicodeLine[i] = NULL;
		if (!PyArg_ParseTuple(args, "i|OOO", &percentage,&unicodeLine[0], &unicodeLine[1], &unicodeLine[2]))	return NULL;

    string utf8Line[3];
    for (int i = 0; i < 3; i++)
    {
      if (unicodeLine[i] && !PyGetUnicodeString(utf8Line[i], unicodeLine[i], i+2))
        return NULL;
    }
  
		CGUIDialogProgress* pDialog= (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
		if (PyWindowIsNull(pDialog)) return NULL;

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
		for (int i = 0; i < 3; i++)
		{
			if (unicodeLine[i])
				pDialog->SetLine(i,utf8Line[i]);
		}
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(isCanceled__doc__,
		"iscanceled() -- Returns True if the user pressed cancel.");

	PyObject* Dialog_ProgressIsCanceled(PyObject *self, PyObject *args)
	{
		bool canceled = false;
		CGUIDialogProgress* pDialog= (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
		if (PyWindowIsNull(pDialog)) return NULL;

		g_graphicsContext.Lock();
		canceled = pDialog->IsCanceled();
		g_graphicsContext.Unlock();

		return Py_BuildValue("b", canceled);
	}

	PyDoc_STRVAR(close__doc__,
		"close() -- Close the dialog.");

	PyObject* Dialog_ProgressClose(PyObject *self, PyObject *args)
	{
		CGUIDialogProgress* pDialog= (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
		if (PyWindowIsNull(pDialog)) return NULL;

		g_graphicsContext.Lock();
		pDialog->Close();
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyMethodDef WindowDialog_methods[] = {
		{NULL, NULL, 0, NULL}
	};

	/* xbmc Dialog functions for use in python */
	PyMethodDef Dialog_methods[] = {
		{"yesno", (PyCFunction)Dialog_YesNo, METH_VARARGS, yesno__doc__},
		{"select", (PyCFunction)Dialog_Select, METH_VARARGS, select__doc__},
		{"ok", (PyCFunction)Dialog_OK, METH_VARARGS, ok__doc__},
		{NULL, NULL, 0, NULL}
	};

	/* xbmc progress Dialog functions for use in python */
	PyMethodDef DialogProgress_methods[] = {
		{"create", (PyCFunction)Dialog_ProgressCreate, METH_VARARGS, create__doc__},
		{"update", (PyCFunction)Dialog_ProgressUpdate, METH_VARARGS, update__doc__},
		{"close", (PyCFunction)Dialog_ProgressClose, METH_VARARGS, close__doc__},
		{"iscanceled", (PyCFunction)Dialog_ProgressIsCanceled, METH_VARARGS, isCanceled__doc__},
		{NULL, NULL, 0, NULL}
	};

	PyDoc_STRVAR(windowDialog__doc__,
		"WindowDialog class.\n");

	PyDoc_STRVAR(dialog__doc__,
		"Dialog class.\n");

	PyDoc_STRVAR(dialogProgress__doc__,
		"DialogProgress class.\n");

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject WindowDialog_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmcgui.WindowDialog",    /*tp_name*/
			sizeof(WindowDialog),      /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			(destructor)Window_Dealloc,/*tp_dealloc*/
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
			windowDialog__doc__,       /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			WindowDialog_methods,      /* tp_methods */
			0,                         /* tp_members */
			0,                         /* tp_getset */
			&Window_Type,              /* tp_base */
			0,                         /* tp_dict */
			0,                         /* tp_descr_get */
			0,                         /* tp_descr_set */
			0,                         /* tp_dictoffset */
			0,                         /* tp_init */
			0,                         /* tp_alloc */
			WindowDialog_New,          /* tp_new */
	};

	PyTypeObject DialogProgress_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmcgui.DialogProgress",  /*tp_name*/
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
			dialogProgress__doc__,     /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			DialogProgress_methods,    /* tp_methods */
			0,                         /* tp_members */
	};

	PyTypeObject Dialog_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmcgui.Dialog",          /*tp_name*/
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
			dialog__doc__,             /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			Dialog_methods,            /* tp_methods */
			0,                         /* tp_members */
	};
}

#ifdef __cplusplus
}
#endif
