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

		PyObject *line = NULL;
		PyObject *heading = NULL;
    if (!PyArg_ParseTuple(args, "|OO", &line, &heading)) return NULL;

    string utf8Line;
    if (line && !PyGetUnicodeString(utf8Line, line, 1)) return NULL;
    string utf8Heading;
    if (heading && !PyGetUnicodeString(utf8Heading, heading, 2)) return NULL;

		self->strDefault = utf8Line;
    self->strHeading = utf8Heading;

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

    pKeyboard->Initialize();
		pKeyboard->CenterWindow();
    pKeyboard->SetHeading(self->strHeading);
		pKeyboard->SetText(CStdString(self->strDefault));

		// do modal of dialog
		ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_KEYBOARD, m_gWindowManager.GetActiveWindow()};
		g_applicationMessenger.SendMessage(tMsg, true);

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(setDefault__doc__,
		"setDefault(string default) -- Set new text that is displayed as default.");

	PyObject* Keyboard_SetDefault(Keyboard *self, PyObject *args)
	{
		PyObject *line = NULL;
		if (!PyArg_ParseTuple(args, "|O", &line))	return NULL;

    string utf8Line;
    if (line && !PyGetUnicodeString(utf8Line, line, 1)) return NULL;
		self->strDefault = utf8Line;

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

	PyDoc_STRVAR(setHeading__doc__,
		"setHeading(string heading) -- Set new text that is displayed as heading.");

	PyObject* Keyboard_SetHeading(Keyboard *self, PyObject *args)
	{
		PyObject *line = NULL;
    if (!PyArg_ParseTuple(args, "|O", &line)) return NULL;

    string utf8Line;
    if (line && !PyGetUnicodeString(utf8Line, line, 1)) return NULL;
		self->strHeading = utf8Line;

		CGUIDialogKeyboard *pKeyboard = (CGUIDialogKeyboard*)m_gWindowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);
		if(!pKeyboard)
		{
			PyErr_SetString(PyExc_SystemError, "Unable to load keyboard");
			return NULL;
		}

		pKeyboard->SetHeading(self->strHeading);

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

		return Py_BuildValue("b", pKeyboard->IsConfirmed());
	}

	PyMethodDef Keyboard_methods[] = {
		{"doModal", (PyCFunction)Keyboard_DoModal, METH_VARARGS, doModal__doc__},
		{"setDefault", (PyCFunction)Keyboard_SetDefault, METH_VARARGS, setDefault__doc__},
		{"setHeading", (PyCFunction)Keyboard_SetHeading, METH_VARARGS, setHeading__doc__},
		{"getText", (PyCFunction)Keyboard_GetText, METH_VARARGS, getText__doc__},
		{"isConfirmed", (PyCFunction)Keyboard_IsConfirmed, METH_VARARGS, isConfirmed__doc__},
		{NULL, NULL, 0, NULL}
	};

	PyDoc_STRVAR(keyboard__doc__,
		"Keyboard class.\n"
		"\n"
    "Keyboard([string default, string heading]) -- Creates a new Keyboard object with default text\n"
		"                              and heading if supplied.");

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject Keyboard_Type;
	
  void initKeyboard_Type()
	{
	  PyInitializeTypeObject(&Keyboard_Type);
	  
	  Keyboard_Type.tp_name = "xbmc.Keyboard";
	  Keyboard_Type.tp_basicsize = sizeof(Keyboard);
	  Keyboard_Type.tp_dealloc = (destructor)Keyboard_Dealloc;
	  Keyboard_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	  Keyboard_Type.tp_doc = keyboard__doc__;
	  Keyboard_Type.tp_methods = Keyboard_methods;
	  Keyboard_Type.tp_base = 0;
	  Keyboard_Type.tp_new = Keyboard_New;
	}
}

#ifdef __cplusplus
}
#endif
