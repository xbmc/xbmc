#include "..\python.h"
#include "..\..\..\application.h"
#include "GuiLabelControl.h"

#define ACTIVE_WINDOW	m_gWindowManager.GetActiveWindow()

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
	typedef struct {
    PyObject_HEAD
		int iWindowId;
		int iOldWindowId;
	} Window;

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

		CGUIWindow* pWindow = new CGUIWindow(id);
		g_graphicsContext.Lock();
		m_gWindowManager.Add(pWindow);
		g_graphicsContext.Unlock();

		return (PyObject*)self;
	}

	void Window_Dealloc(Window* self)
	{
		CGUIWindow* pWindow;
		g_graphicsContext.Lock();
		pWindow = m_gWindowManager.GetWindow(self->iWindowId);
		m_gWindowManager.Remove(self->iWindowId);
		g_graphicsContext.Unlock();
		pWindow->FreeResources();
		delete pWindow;

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
		self->iOldWindowId = ACTIVE_WINDOW;

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
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Window_AddLabel(Window *self, PyObject *args)
	{ 
		//const char *cLine[4];
		//CStdStringW wstrLine[4];

		CGUIWindow* pWindow = (CGUIWindow*)m_gWindowManager.GetWindow(self->iWindowId);
		if (!pWindow) return NULL;

		//for (int i = 0; i < 4; i++)	cLine[i] = NULL;
		// get lines, last 2 lines are optional.
		//if (!PyArg_ParseTuple(args, "ss|ss", &cLine[0], &cLine[1], &cLine[2], &cLine[3]))	return NULL;

		// convert char strings to wchar strings and set the header + line 1, 2 and 3 for dialog
		/*for (int i = 0; i < 4; i++)
		{
			if (cLine[i]) wstrLine[i] = cLine[i];
			else wstrLine[i] = L"";
		}
	*/
		CGUIControl* pControl = new CGUILabelControl(ACTIVE_WINDOW ,
				4500, 100, 100, 200, 200,
				"font13", L"python test label", 0xFFFFFFFF, 0, false);

		g_graphicsContext.Lock();
		pWindow->Add(pControl);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyMethodDef Window_methods[] = {
		{"load", (PyCFunction)Window_Load, METH_VARARGS, ""},
		{"show", (PyCFunction)Window_Show, METH_VARARGS, ""},
		{"close", (PyCFunction)Window_Close, METH_VARARGS, ""},
		{"addlabel", (PyCFunction)Window_AddLabel, METH_VARARGS, ""},
		{NULL, NULL, 0, NULL}
	};

	PyTypeObject WindowType = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmc.Window",             /*tp_name*/
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
