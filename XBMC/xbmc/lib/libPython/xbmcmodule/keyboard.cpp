#include "../../../stdafx.h"
#include "keyboard.h"
#include "pyutil.h"
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

	PyDoc_STRVAR(doModal__doc__,
		"doModal() -- Show keyboard and wait for user action.");

	PyObject* Keyboard_DoModal(Keyboard *self, PyObject *args)
	{
		CGUIDialogKeyboard *pKeyboard = (CGUIDialogKeyboard*)m_gWindowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);
		if(!pKeyboard)
		{
			PyErr_SetString(PyExc_SystemError, "Unable to load virtual keyboard");
			return NULL;
		}

		pKeyboard->CenterWindow();
		pKeyboard->SetText(CStdString(self->strDefault));

		// do modal of dialog
		ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_KEYBOARD, m_gWindowManager.GetActiveWindow()};
		g_applicationMessenger.SendMessage(tMsg, true);

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(setDefault__doc__,
		"setDefault(string text) -- Set new text that is displayed as default.");

	PyObject* Keyboard_SetDefault(Keyboard *self, PyObject *args)
	{
		char *cLine = NULL;
		if (!PyArg_ParseTuple(args, "|s", &cLine))	return NULL;

		self->strDefault = cLine ? cLine : "";

		CGUIDialogKeyboard *pKeyboard = (CGUIDialogKeyboard*)m_gWindowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);
		if(!pKeyboard)
		{
			PyErr_SetString(PyExc_SystemError, "Unable to load keyboard");
			return NULL;
		}

		pKeyboard->SetText(CStdString(self->strDefault));

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(getText__doc__,
		"getText() -- Returns the user input.\n"
		"\n"
		"This will only succeed if the user entered some text or if there is a default\n"
		"text set for this Keyboard.");

	PyObject* Keyboard_GetText(Keyboard *self, PyObject *args)
	{
		CGUIDialogKeyboard *pKeyboard = (CGUIDialogKeyboard*)m_gWindowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);
		if(!pKeyboard)
		{
			PyErr_SetString(PyExc_SystemError, "Unable to load keyboard");
			return NULL;
		}

		return Py_BuildValue("s", pKeyboard->GetText().c_str());
	}

	PyDoc_STRVAR(isConfirmed__doc__,
		"isConfirmed() -- Returns False if the user cancelled the input.");

	PyObject* Keyboard_IsConfirmed(Keyboard *self, PyObject *args)
	{
		CGUIDialogKeyboard *pKeyboard = (CGUIDialogKeyboard*)m_gWindowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);
		if(!pKeyboard)
		{
			PyErr_SetString(PyExc_SystemError, "Unable to load keyboard");
			return NULL;
		}

		return Py_BuildValue("b", pKeyboard->IsDirty());
	}

	PyMethodDef Keyboard_methods[] = {
		{"doModal", (PyCFunction)Keyboard_DoModal, METH_VARARGS, doModal__doc__},
		{"setDefault", (PyCFunction)Keyboard_SetDefault, METH_VARARGS, setDefault__doc__},
		{"getText", (PyCFunction)Keyboard_GetText, METH_VARARGS, getText__doc__},
		{"isConfirmed", (PyCFunction)Keyboard_IsConfirmed, METH_VARARGS, isConfirmed__doc__},
		{NULL, NULL, 0, NULL}
	};

	PyDoc_STRVAR(keyboard__doc__,
		"Keyboard class.\n"
		"\n"
		"Keyboard([string default]) -- Creates a new Keyboard object with default text\n"
		"                              if supplied.");

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
			keyboard__doc__,           /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			Keyboard_methods,          /* tp_methods */
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
