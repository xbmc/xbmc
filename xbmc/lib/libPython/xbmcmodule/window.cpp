#include "stdafx.h"
#include "window.h"
#include "guiwindowmanager.h"
#include "GuiLabelControl.h"
#include "GuiListControl.h"
#include "GuiFadeLabelControl.h"
#include "GuiTextBox.h"

#define ACTIVE_WINDOW	m_gWindowManager.GetActiveWindow()

using namespace std;

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

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

		self->iWindowId = -1;

		if (!PyArg_ParseTuple(args, "|i", &self->iWindowId)) return NULL;

		if (self->iWindowId != -1)
		{
			// user specified window id, use this one if it exists
			// It is not possible to capture key presses or button presses
			self->pWindow = m_gWindowManager.GetWindow(self->iWindowId);
			if (!self->pWindow)
			{
				PyErr_SetString((PyObject*)self, "Window id does not exist");
				self->ob_type->tp_free((PyObject*)self);
				return NULL;
			}
			self->iOldWindowId = 0;
			self->bModal = false;
			self->iCurrentControlId = 3000;
			self->bIsPythonWindow = false;
		}
		else
		{
			// window id's 3000 - 3100 are reserved for python
			// get first window id that is not in use
			int id = 3000;
			// if window 3099 is in use it means python can't create more windows
			if (m_gWindowManager.GetWindow(3099))
			{
				PyErr_SetString((PyObject*)self, "maximum number of windows reached");
				self->ob_type->tp_free((PyObject*)self);
				return NULL;
			}
			while(id < 3100 && m_gWindowManager.GetWindow(id) != NULL) id++;
			self->iWindowId = id;
			self->iOldWindowId = 0;
			self->bModal = false;
			self->bIsPythonWindow = true;
			self->pWindow = new CGUIPythonWindow(id);

			if (self->bIsPythonWindow)
				((CGUIPythonWindow*)self->pWindow)->SetCallbackWindow((PyObject*)self);

			g_graphicsContext.Lock();
			m_gWindowManager.Add(self->pWindow);
			g_graphicsContext.Unlock();
		}

		return (PyObject*)self;
	}

	void Window_Dealloc(Window* self)
	{
		g_graphicsContext.Lock();
		if (self->bIsPythonWindow)
		{
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
		}

		// free all recources in use by controls
		std::vector<Control*>::iterator it = self->vecControls.begin();
		while (it != self->vecControls.end())
		{
			Control* pControl = *it;

			// initialize control to zero
			self->pWindow->Remove(pControl->iControlId);
			pControl->pGUIControl->FreeResources();
			delete pControl->pGUIControl;
			pControl->pGUIControl = NULL;
			pControl->iControlId = 0;
			pControl->iParentId = 0;
			Py_DECREF(pControl);

			++it;
		}

		g_graphicsContext.Unlock();
		if (self->bIsPythonWindow) delete self->pWindow;
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
		if (self->iOldWindowId != self->iWindowId &&
				self->iWindowId != ACTIVE_WINDOW)
			self->iOldWindowId = ACTIVE_WINDOW;

		g_graphicsContext.Lock();
		m_gWindowManager.ActivateWindow(self->iWindowId);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Window_Close(Window *self, PyObject *args)
	{
		self->bModal = false;
		if (self->bIsPythonWindow)
			((CGUIPythonWindow*)self->pWindow)->PulseActionEvent();

		g_graphicsContext.Lock();
		m_gWindowManager.ActivateWindow(self->iOldWindowId);
		self->iOldWindowId = 0;
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Window_OnAction(Window *self, PyObject *args)
	{
		DWORD action;
		if (!PyArg_ParseTuple(args, "l", &action)) return NULL;

		if(action == ACTION_PREVIOUS_MENU)
		{
			Window_Close(self, args);
		}
		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Window_DoModal(Window *self, PyObject *args)
	{
		if (self->bIsPythonWindow)
		{
			self->bModal = true;

			if(self->iWindowId != ACTIVE_WINDOW) Window_Show(self, NULL);

			while(self->bModal)
			{
				Py_BEGIN_ALLOW_THREADS
				((CGUIPythonWindow*)self->pWindow)->WaitForActionEvent(INFINITE);
				Py_END_ALLOW_THREADS

				// only call Py_MakePendingCalls from a python thread
				Py_MakePendingCalls();
			}
		}
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
					pControlLabel->strFont, pControlLabel->strText, pControlLabel->dwTextColor, pControlLabel->dwTextColor, 0, false);
		}

		// Control Fade Label
		else if (!strcmp(pControl->ob_type->tp_name, ControlFadeLabel_Type.tp_name))
		{
			ControlFadeLabel* pControlFadeLabel = (ControlFadeLabel*)pControl;
			pControl->pGUIControl = new CGUIFadeLabelControl(pControl->iParentId, pControl->iControlId,
					pControl->dwPosX, pControl->dwPosY, pControl->dwWidth, pControl->dwHeight,
					pControlFadeLabel->strFont, pControlFadeLabel->dwTextColor, 0);

			CGUIMessage msg(GUI_MSG_LABEL_RESET, pControl->iParentId, pControl->iControlId);
			pControl->pGUIControl->OnMessage(msg);
		}

		// Control TextBox
		else if (!strcmp(pControl->ob_type->tp_name, ControlTextBox_Type.tp_name))
		{
			ControlTextBox* pControlTextBox = (ControlTextBox*)pControl;

			// create textbox
			pControl->pGUIControl = new CGUITextBox(pControl->iParentId, pControl->iControlId,
					pControl->dwPosX, pControl->dwPosY, pControl->dwWidth, pControl->dwHeight,
					pControlTextBox->strFont, pControlTextBox->pControlSpin->dwWidth, pControlTextBox->pControlSpin->dwHeight,
					pControlTextBox->pControlSpin->strTextureUp, pControlTextBox->pControlSpin->strTextureDown, pControlTextBox->pControlSpin->strTextureUpFocus,
					pControlTextBox->pControlSpin->strTextureDownFocus, pControlTextBox->pControlSpin->dwColor, pControlTextBox->pControlSpin->dwPosX,
					pControlTextBox->pControlSpin->dwPosY, pControlTextBox->strFont, pControlTextBox->dwTextColor);

			// reset textbox
			CGUIMessage msg(GUI_MSG_LABEL_RESET, pControl->iParentId, pControl->iControlId);
			pControl->pGUIControl->OnMessage(msg);

			// set values for spincontrol
			pControlTextBox->pControlSpin->iControlId = pControl->iControlId;
			pControlTextBox->pControlSpin->iParentId = pControl->iParentId;
		}

		// Control Button
		else if (!strcmp(pControl->ob_type->tp_name, ControlButton_Type.tp_name))
		{
			ControlButton* pControlButton = (ControlButton*)pControl;
			pControl->pGUIControl = new CGUIButtonControl(pControl->iParentId, pControl->iControlId,
					pControl->dwPosX, pControl->dwPosY, pControl->dwWidth, pControl->dwHeight,
					pControlButton->strTextureFocus, pControlButton->strTextureNoFocus);

			CGUIButtonControl* pGuiButtonControl = (CGUIButtonControl*)pControl->pGUIControl;

			pGuiButtonControl->SetLabel(pControlButton->strFont, pControlButton->strText, pControlButton->dwTextColor);
			pGuiButtonControl->SetDisabledColor(pControlButton->dwDisabledColor);
		}

		// Image
		else if (!strcmp(pControl->ob_type->tp_name, ControlImage_Type.tp_name))
		{
			ControlImage* pControlImage = (ControlImage*)pControl;
			pControl->pGUIControl = new CGUIImage(pControl->iParentId, pControl->iControlId,
					pControl->dwPosX, pControl->dwPosY, pControl->dwWidth, pControl->dwHeight,
					pControlImage->strFileName, pControlImage->strColorKey);
		}

		// Control List
		else if (!strcmp(pControl->ob_type->tp_name, ControlList_Type.tp_name))
		{
			ControlList* pControlList = (ControlList*)pControl;
			pControl->pGUIControl = new CGUIListControl(pControl->iParentId, pControl->iControlId,
					pControl->dwPosX, pControl->dwPosY, pControl->dwWidth, pControl->dwHeight,
					pControlList->strFont, pControlList->dwSpinWidth, pControlList->dwSpinHeight,
					pControlList->strTextureUp, pControlList->strTextureDown, pControlList->strTextureUpFocus,
					pControlList->strTextureDownFocus, pControlList->dwSpinColor, pControlList->dwSpinX,
					pControlList->dwSpinY, pControlList->strFont,pControlList->dwTextColor,
					pControlList->dwSelectedColor, pControlList->strButton, pControlList->strButtonFocus);
			/*		pControl->SetNavigation(up,down,left,right);
						pControl->SetColourDiffuse(dwColorDiffuse);
						pControl->SetScrollySuffix(strSuffix);
						pControl->SetTextOffsets(iTextXOff,iTextYOff, iTextXOff2,iTextYOff2);
						pControl->SetVisible(bVisible);
						pControl->SetImageDimensions(dwitemWidth, dwitemHeight);
						pControl->SetItemHeight(iTextureHeight);
						pControl->SetSpace(iSpace);
						pControl->SetColors2(dwTextColor2, dwSelectedColor2);
						pControl->SetFont2( strFont2 );
						*/
				//((CGUIListControl*)pControl->pGUIControl)->
		}

		Py_INCREF(pControl);

		// set default navigation for control
		pControl->iControlUp = pControl->iControlId;
		pControl->iControlDown = pControl->iControlId;
		pControl->iControlLeft = pControl->iControlId;
		pControl->iControlRight = pControl->iControlId;

		pControl->pGUIControl->SetNavigation(pControl->iControlUp,
				pControl->iControlDown,	pControl->iControlLeft, pControl->iControlRight);

		g_graphicsContext.Lock();

		// add control to list and allocate recources for the control
		self->vecControls.push_back(pControl);
		pControl->pGUIControl->AllocResources();
		pWindow->Add(pControl->pGUIControl);

		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Window_SetFocus(Window *self, PyObject *args)
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

		g_graphicsContext.Lock();
		pWindow->OnMessage(CGUIMessage(GUI_MSG_SETFOCUS,pControl->iParentId, pControl->iControlId));
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

	PyObject* Window_GetHeight(Window *self, PyObject *args)
	{
		return PyLong_FromLong(g_graphicsContext.GetHeight());
	}

	PyObject* Window_GetWidth(Window *self, PyObject *args)
	{ 
		return PyLong_FromLong(g_graphicsContext.GetWidth());
	}

	PyMethodDef Window_methods[] = {
		//{"load", (PyCFunction)Window_Load, METH_VARARGS, ""},
		{"onAction", (PyCFunction)Window_OnAction, METH_VARARGS, ""},
		{"doModal", (PyCFunction)Window_DoModal, METH_VARARGS, ""},
		{"show", (PyCFunction)Window_Show, METH_VARARGS, ""},
		{"close", (PyCFunction)Window_Close, METH_VARARGS, ""},
		{"addControl", (PyCFunction)Window_AddControl, METH_VARARGS, ""},
		{"removeControl", (PyCFunction)Window_RemoveControl, METH_VARARGS, ""},
		{"setFocus", (PyCFunction)Window_SetFocus, METH_VARARGS, ""},
		{"getHeight", (PyCFunction)Window_GetHeight, METH_VARARGS, ""},
		{"getWidth", (PyCFunction)Window_GetWidth, METH_VARARGS, ""},
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
