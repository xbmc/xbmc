#include "window.h"
#include "control.h"
#include "..\..\..\application.h"
#include "GuiLabelControl.h"

#define ACTIVE_WINDOW	m_gWindowManager.GetActiveWindow()

using namespace std;

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

/*****************************************************************
 * start of window methods and python objects
 *****************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
	PyObject* Window_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		Window *self;

		self = (Window*)type->tp_alloc(type, 0);
		if (!self) return NULL;

		// window id's 3000 - 3100 are reserved for python
		// get first window id that is not in use
		int id = 3000;
		// if window 3099 is in use it means python can't create more windows
		if (m_gWindowManager.GetWindow(3099))
		{
			PyErr_SetString((PyObject*)self, "maximum number of windows reached");
			return NULL;
		}
		while(id < 3100 && m_gWindowManager.GetWindow(id) != NULL) id++;
		self->iWindowId = id;
		self->iOldWindowId = 0;
		self->bIsCreatedByPython = true;
		self->pWindow = m_gWindowManager.GetWindow(id);

		CGUIWindow* pWindow = new CGUIWindow(id);

		g_graphicsContext.Lock();
		m_gWindowManager.Add(pWindow);
		g_graphicsContext.Unlock();

		return (PyObject*)self;
	}

	void Window_Dealloc(Window* self)
	{
		g_graphicsContext.Lock();
		// first change to an existing window
		if (ACTIVE_WINDOW == self->iWindowId)
		{
			if(m_gWindowManager.GetWindow(self->iOldWindowId))
			{
				m_gWindowManager.ActivateWindow(self->iOldWindowId);
			}
			// old window does not exist anymore, switch to home
			else m_gWindowManager.ActivateWindow(0);
		}

		// free all recources in use by controls
		vector<Control*>::iterator it = self->vecControls.begin();
		while (it != self->vecControls.end())
		{
			Control* pControl = *it;

			// initialize control to zero
			pControl->pGUIControl->FreeResources();
			delete pControl->pGUIControl;
			pControl->pGUIControl = NULL;
			pControl->iControlId = 0;
			pControl->iParentId = 0;
			Py_DECREF(pControl);

			++it;
		}

		g_graphicsContext.Unlock();

		self->ob_type->tp_free((PyObject*)self);
	}

	PyObject* Window_Load(Window *self, PyObject *args)
	{
		CGUIWindow* pWindow = (CGUIWindow*)m_gWindowManager.GetWindow(self->iWindowId);
		if (!pWindow) return NULL;

		char *cLine;
		if (!PyArg_ParseTuple(args, "s", &cLine)) return NULL;
		if (!pWindow->Load(cLine))
		{
			// error loading file. Since window id is now set to 9999 we set it to
			// the old value again
			g_graphicsContext.Lock();
			pWindow->SetID(self->iWindowId);
			g_graphicsContext.Unlock();

			char error[1024];
			strcpy(error, "could not load: ");
			strcat(error, cLine);
			PyErr_SetString((PyObject*)self, error);
			return NULL;
		}
		// load succeeded, just in case window id is specified in xml file, we set it
		// to our own value here
		g_graphicsContext.Lock();
		pWindow->SetID(self->iWindowId);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Window_Show(Window *self, PyObject *args)
	{
		if (self->iOldWindowId != self->iWindowId) self->iOldWindowId = ACTIVE_WINDOW;

		g_graphicsContext.Lock();
		m_gWindowManager.ActivateWindow(self->iWindowId);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Window_Close(Window *self, PyObject *args)
	{ 
		g_graphicsContext.Lock();
		m_gWindowManager.ActivateWindow(self->iOldWindowId);
		self->iOldWindowId = 0;
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Window_AddControl(Window *self, PyObject *args)
	{ 
		CGUIWindow* pWindow = (CGUIWindow*)m_gWindowManager.GetWindow(self->iWindowId);
		if (!pWindow) return NULL;

		Control* pControl;
		if (!PyArg_ParseTuple(args, "O", &pControl)) return NULL;
		// type checking, object should be of type Control
		if(strcmp(((PyObject*)pControl)->ob_type->tp_base->tp_name, Control_Type.tp_name))
		{
			PyErr_SetString((PyObject*)self, "Object should be of type Control");
			return NULL;
		}

		if(pControl->iControlId != 0)
		{
			PyErr_SetString((PyObject*)self, "Control is already used");
			return NULL;
		}

		pControl->iParentId = self->iWindowId;
		// assign control id, if id is already in use, try next id
		do pControl->iControlId = ++self->iCurrentControlId;
		while (pWindow->GetControl(pControl->iControlId));

		// Control Label
		if (!strcmp(pControl->ob_type->tp_name, ControlLabel_Type.tp_name))
		{
			ControlLabel* pControlLabel = (ControlLabel*)pControl;
			pControl->pGUIControl = new CGUILabelControl(pControl->iParentId, pControl->iControlId,
					pControl->dwPosX, pControl->dwPosY, pControl->dwWidth, pControl->dwHeight,
					pControlLabel->strFont, pControlLabel->strText, pControlLabel->dwTextColor, 0, false);
		}

		// Image
		else if (!strcmp(pControl->ob_type->tp_name, ControlImage_Type.tp_name))
		{
			ControlImage* pControlImage = (ControlImage*)pControl;
			pControl->pGUIControl = new CGUIImage(pControl->iParentId, pControl->iControlId,
					pControl->dwPosX, pControl->dwPosY, pControl->dwWidth, pControl->dwHeight,
					pControlImage->strFileName, pControlImage->strColorKey);
		}

		// add control to list and allocate recources for the control
		Py_INCREF(pControl);
		self->vecControls.push_back(pControl);
		pControl->pGUIControl->AllocResources();

		g_graphicsContext.Lock();
		pWindow->Add(pControl->pGUIControl);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Window_RemoveControl(Window *self, PyObject *args)
	{ 
		CGUIWindow* pWindow = (CGUIWindow*)m_gWindowManager.GetWindow(self->iWindowId);
		if (!pWindow) return NULL;

		Control* pControl;
		if (!PyArg_ParseTuple(args, "O", &pControl)) return NULL;
		// type checking, object should be of type Control
		if(strcmp(((PyObject*)pControl)->ob_type->tp_base->tp_name, Control_Type.tp_name))
		{
			PyErr_SetString((PyObject*)self, "Object should be of type Control");
			return NULL;
		}

		if(!pWindow->GetControl(pControl->iControlId))
		{
			PyErr_SetString((PyObject*)self, "Control does not exist in window");
			return NULL;
		}

		// delete control from vecControls in window object
		vector<Control*>::iterator it = self->vecControls.begin();
		while (it != self->vecControls.end())
		{
      Control* control = *it;
			if (control->iControlId == pControl->iControlId)
			{
				it = self->vecControls.erase(it);
			} else ++it;
		}

		g_graphicsContext.Lock();

		pWindow->Remove(pControl->iControlId);
		pControl->pGUIControl->FreeResources();
		delete pControl->pGUIControl;

		// initialize control to zero
		pControl->pGUIControl = NULL;
		pControl->iControlId = 0;
		pControl->iParentId = 0;
		Py_DECREF(pControl);
		g_graphicsContext.Unlock();



		Py_INCREF(Py_None);
		return Py_None;
	}

	PyMethodDef Window_methods[] = {
		//{"load", (PyCFunction)Window_Load, METH_VARARGS, ""},
		{"show", (PyCFunction)Window_Show, METH_VARARGS, ""},
		{"close", (PyCFunction)Window_Close, METH_VARARGS, ""},
		{"addControl", (PyCFunction)Window_AddControl, METH_VARARGS, ""},
		{"removeControl", (PyCFunction)Window_RemoveControl, METH_VARARGS, ""},
		{NULL, NULL, 0, NULL}
	};

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject Window_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmcgui.Window",         /*tp_name*/
			sizeof(Window),            /*tp_basicsize*/
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
			"Window Objects",          /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			Window_methods,            /* tp_methods */
			0,                         /* tp_members */
			0,                         /* tp_getset */
			0,                         /* tp_base */
			0,                         /* tp_dict */
			0,                         /* tp_descr_get */
			0,                         /* tp_descr_set */
			0,                         /* tp_dictoffset */
			0,                         /* tp_init */
			0,                         /* tp_alloc */
			Window_New,                /* tp_new */
	};
}

#ifdef __cplusplus
}
#endif
