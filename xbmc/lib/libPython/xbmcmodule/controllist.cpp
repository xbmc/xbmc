#include "stdafx.h"
#include "..\python.h"
#include "GuiListControl.h"
#include "control.h"
#include "pyutil.h"

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
	extern PyObject* ControlSpin_New(void);


	PyObject* ControlList_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		ControlList *self;
		char *cFont = NULL;
		char *cTextColor = NULL;
		char *cTextureButton = NULL;
		char *cTextureButtonFocus = NULL;
		
		self = (ControlList*)type->tp_alloc(type, 0);
		if (!self) return NULL;
		
		// create a python spin control
		self->pControlSpin = (ControlSpin*)ControlSpin_New();
		if (!self->pControlSpin) return NULL;

		if (!PyArg_ParseTuple(args, "llll|ss", &self->dwPosX, &self->dwPosY, &self->dwWidth, &self->dwHeight,
			&cFont, &cTextColor, &cTextureButton, &cTextureButtonFocus)) return NULL;

		// set default values if needed
		self->strFont = cFont ? cFont : "font13";

		if (cTextColor) sscanf(cTextColor, "%x", &self->dwTextColor);
		else self->dwTextColor = 0xe0f0f0f0;//0xffffffff;

		self->dwSelectedColor = 0xffffffff;//0xFFF8BC70;

		self->strTextureButton = cTextureButton ? cTextureButton : "list-nofocus.png";		
		self->strTextureButtonFocus = cTextureButtonFocus ? cTextureButtonFocus : "list-focus.png";	

		self->dwImageHeight = 10;
		self->dwImageWidth = 10;
		self->dwItemHeight = 27;
		self->dwSpace = 2;

		// default values for spin control
		self->pControlSpin->dwPosX = self->dwPosX + self->dwWidth - 35;
		self->pControlSpin->dwPosY = self->dwPosY + self->dwHeight - 15;

		return (PyObject*)self;
	}

	void ControlList_Dealloc(ControlList* self)
	{
		// delete spincontrol
		Py_DECREF(self->pControlSpin);

		// delete all ListItem from vector
		vector<ListItem*>::iterator it = self->vecItems.begin();
		while (it != self->vecItems.end())
		{
      ListItem* pListItem = *it;
			Py_DECREF(pListItem);
			++it;
		}
		self->vecItems.clear();

		self->ob_type->tp_free((PyObject*)self);
	}

	/*
	 * ControlList_AddItem
	 * (string label) / (ListItem)
	 * ListItem is added to vector
	 * For a string we create a new ListItem and add it to the vector
	 */
	PyObject* ControlList_AddItem(ControlList *self, PyObject *args)
	{
		PyObject *pObject;
		wstring strText;
		
		ListItem* pListItem = NULL;

		if (!PyArg_ParseTuple(args, "O", &pObject))	return NULL;
		if (ListItem_CheckExact(pObject))
		{
			// object is a listitem
			pListItem = (ListItem*)pObject;
			Py_INCREF(pListItem);
		}
		else
		{
			// object is probably a text item
			if (!PyGetUnicodeString(strText, pObject, 1)) return NULL;
			// object is a unicode string now, create a new ListItem
			pListItem = ListItem_FromString(strText);
		}

		// add item to objects vector
		self->vecItems.push_back(pListItem);

		// create message
		CGUIMessage msg(GUI_MSG_LABEL_ADD, self->iParentId, self->iControlId);
		msg.SetLPVOID(pListItem->item);

		// send message
		PyGUILock();
		if (self->pGUIControl) self->pGUIControl->OnMessage(msg);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* ControlList_Reset(ControlList *self, PyObject *args)
	{
		// create message
		ControlList *pControl = (ControlList*)self;
		CGUIMessage msg(GUI_MSG_LABEL_RESET, pControl->iParentId, pControl->iControlId);

		// send message
		PyGUILock();
		if (pControl->pGUIControl) pControl->pGUIControl->OnMessage(msg);
		PyGUIUnlock();

		// delete all items from vector
		// delete all ListItem from vector
		vector<ListItem*>::iterator it = self->vecItems.begin();
		while (it != self->vecItems.end())
		{
      ListItem* pListItem = *it;
			Py_DECREF(pListItem);
			++it;
		}
		self->vecItems.clear();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* ControlList_GetSpinControl(ControlTextBox *self, PyObject *args)
	{
		Py_INCREF(self->pControlSpin);
		return (PyObject*)self->pControlSpin;
	}

	PyObject* ControlList_SetImageDimensions(ControlList *self, PyObject *args)
	{
		if (!PyArg_ParseTuple(args, "ll", &self->dwImageWidth, &self->dwImageHeight)) return NULL;

		PyGUILock();
		CGUIListControl* pListControl = (CGUIListControl*) self->pGUIControl;
		pListControl->SetImageDimensions(self->dwImageWidth, self->dwImageHeight);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* ControlList_SetItemHeight(ControlList *self, PyObject *args)
	{
		if (!PyArg_ParseTuple(args, "l", &self->dwItemHeight)) return NULL;

		PyGUILock();
		CGUIListControl* pListControl = (CGUIListControl*) self->pGUIControl;
		pListControl->SetItemHeight(self->dwItemHeight);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* ControlList_SetSpace(ControlList *self, PyObject *args)
	{
		if (!PyArg_ParseTuple(args, "l", &self->dwSpace)) return NULL;

		PyGUILock();
		CGUIListControl* pListControl = (CGUIListControl*) self->pGUIControl;
		pListControl->SetSpace(self->dwSpace);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}
				
	PyObject* ControlList_GetSelectedPosition(ControlList *self, PyObject *args)
	{
		// create message
		ControlList *pControl = (ControlList*)self;
		CGUIMessage msg(GUI_MSG_ITEM_SELECTED, pControl->iParentId, pControl->iControlId);

		// send message
		PyGUILock();
		if (pControl->pGUIControl) pControl->pGUIControl->OnMessage(msg);
		PyGUIUnlock();

		return Py_BuildValue("l", msg.GetParam1());
	}

	PyObject* ControlList_GetSelectedItem(ControlList *self, PyObject *args)
	{
		// create message
		ControlList *pControl = (ControlList*)self;
		CGUIMessage msg(GUI_MSG_ITEM_SELECTED, pControl->iParentId, pControl->iControlId);

		// send message
		PyGUILock();
		if (pControl->pGUIControl) pControl->pGUIControl->OnMessage(msg);
		PyGUIUnlock();

		// iterate through itemvector
		ListItem* pListItem = self->vecItems[msg.GetParam1()];
		Py_INCREF(pListItem);

		return (PyObject*)pListItem;
	}
			
	PyMethodDef ControlList_methods[] = {
		{"addItem", (PyCFunction)ControlList_AddItem, METH_VARARGS, ""},
		{"reset", (PyCFunction)ControlList_Reset, METH_VARARGS, ""},
		{"getSpinControl", (PyCFunction)ControlList_GetSpinControl, METH_VARARGS, ""},
		{"getSelectedPosition", (PyCFunction)ControlList_GetSelectedPosition, METH_VARARGS, ""},
		{"getSelectedItem", (PyCFunction)ControlList_GetSelectedItem, METH_VARARGS, ""},
		{"setImageDimensions", (PyCFunction)ControlList_SetImageDimensions, METH_VARARGS, ""},
		{"setItemHeight", (PyCFunction)ControlList_SetItemHeight, METH_VARARGS, ""},
		{"setSpace", (PyCFunction)ControlList_SetSpace, METH_VARARGS, ""},
		{NULL, NULL, 0, NULL}
	};

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject ControlList_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmcgui.ControlList",    /*tp_name*/
			sizeof(ControlList),      /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			(destructor)ControlList_Dealloc,/*tp_dealloc*/
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
			"ControlList Objects",    /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			ControlList_methods,      /* tp_methods */
			0,                         /* tp_members */
			0,                         /* tp_getset */
			&Control_Type,             /* tp_base */
			0,                         /* tp_dict */
			0,                         /* tp_descr_get */
			0,                         /* tp_descr_set */
			0,                         /* tp_dictoffset */
			0,                         /* tp_init */
			0,                         /* tp_alloc */
			ControlList_New,          /* tp_new */
	};
}

#ifdef __cplusplus
}
#endif
