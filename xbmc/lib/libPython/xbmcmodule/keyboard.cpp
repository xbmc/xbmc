#include "stdafx.h"
#include "keyboard.h"
#include "GUIWindowManager.h"
#include "..\..\..\keyboard\virtualkeyboard.h"
#include "..\..\..\ApplicationMessenger.h"
#include "..\..\..\util.h"

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
	PyObject* Keyboard_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		Keyboard *self;

		self = (Keyboard*)type->tp_alloc(type, 0);
		if (!self) return NULL;

		char *cLine = NULL;
		if (!PyArg_ParseTuple(args, "|s", &cLine))	return NULL;

		self->strDefault = cLine ? cLine : "";

		return (PyObject*)self;
	}

	void Keyboard_Dealloc(Keyboard* self)
	{
		self->ob_type->tp_free((PyObject*)self);
	}

	PyObject* Keyboard_DoModal(Keyboard *self, PyObject *args)
	{
		CXBVirtualKeyboard* pKeyboard = (CXBVirtualKeyboard*)m_gWindowManager.GetWindow(WINDOW_VIRTUAL_KEYBOARD);
		if(!pKeyboard)
		{
			PyErr_SetString(PyExc_SystemError, "Unable to load virtual keyboard");
			return NULL;
		}

		pKeyboard->Reset();

		WCHAR wsString[1024];
		swprintf(wsString,L"%S", self->strDefault.c_str());
		pKeyboard->SetText(wsString);

		// do modal of dialog
		ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_VIRTUAL_KEYBOARD, m_gWindowManager.GetActiveWindow()};
		g_applicationMessenger.SendMessage(tMsg, true);	

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Keyboard_SetDefault(Keyboard *self, PyObject *args)
	{
		char *cLine = NULL;
		if (!PyArg_ParseTuple(args, "|s", &cLine))	return NULL;

		self->strDefault = cLine ? cLine : "";

		CXBVirtualKeyboard* pKeyboard = (CXBVirtualKeyboard*)m_gWindowManager.GetWindow(WINDOW_VIRTUAL_KEYBOARD);
		if(!pKeyboard)
		{
			PyErr_SetString(PyExc_SystemError, "Unable to load keyboard");
			return NULL;
		}

		WCHAR wsString[1024];
		swprintf(wsString,L"%S", self->strDefault.c_str());
		pKeyboard->SetText(wsString);

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Keyboard_GetText(Keyboard *self, PyObject *args)
	{
		CXBVirtualKeyboard* pKeyboard = (CXBVirtualKeyboard*)m_gWindowManager.GetWindow(WINDOW_VIRTUAL_KEYBOARD);
		if(!pKeyboard)
		{
			PyErr_SetString(PyExc_SystemError, "Unable to load keyboard");
			return NULL;
		}

		CStdString strAnsi;
		CUtil::Unicode2Ansi(pKeyboard->GetText(),strAnsi);

		return Py_BuildValue("s", strAnsi.c_str());
	}

	PyObject* Keyboard_IsConfirmed(Keyboard *self, PyObject *args)
	{
		CXBVirtualKeyboard* pKeyboard = (CXBVirtualKeyboard*)m_gWindowManager.GetWindow(WINDOW_VIRTUAL_KEYBOARD);
		if(!pKeyboard)
		{
			PyErr_SetString(PyExc_SystemError, "Unable to load keyboard");
			return NULL;
		}

		return Py_BuildValue("b", pKeyboard->IsConfirmed());
	}

	PyMethodDef Keyboard_methods[] = {
		{"doModal", (PyCFunction)Keyboard_DoModal, METH_VARARGS, ""},
		{"setDefault", (PyCFunction)Keyboard_SetDefault, METH_VARARGS, ""},
		{"getText", (PyCFunction)Keyboard_GetText, METH_VARARGS, ""},
		{"isConfirmed", (PyCFunction)Keyboard_IsConfirmed, METH_VARARGS, ""},
		{NULL, NULL, 0, NULL}
	};

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject Keyboard_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmc.Keyboard",             /*tp_name*/
			sizeof(Keyboard),            /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			(destructor)Keyboard_Dealloc,/*tp_dealloc*/
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
			"Keyboard Objects",          /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			Keyboard_methods,            /* tp_methods */
			0,                         /* tp_members */
			0,                         /* tp_getset */
			0,                         /* tp_base */
			0,                         /* tp_dict */
			0,                         /* tp_descr_get */
			0,                         /* tp_descr_set */
			0,                         /* tp_dictoffset */
			0,                         /* tp_init */
			0,                         /* tp_alloc */
			Keyboard_New,                /* tp_new */
	};
}

#ifdef __cplusplus
}
#endif
