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

		// if texture is supplied use it, else get default ones
		self->strTextureButton = cTextureButton ? cTextureButton : GetDefaultImage("listcontrol", "textureNoFocus", "list-nofocus.png");		
		self->strTextureButtonFocus = cTextureButtonFocus ? cTextureButtonFocus : GetDefaultImage("listcontrol", "textureFocus", "list-focus.png");	

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
PyDoc_STRVAR(addItem__doc__,
		"addItem(item) -- Add a new item to this control list.\n"
		"\n"
		"item can be a string / unicode string or a ListItem.");

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

	PyDoc_STRVAR(reset__doc__,
		"reset() -- Clear all ListItems in this control list.");

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

	PyDoc_STRVAR(getSpinControl__doc__,
		"getSpinControl() -- returns the associated ControlSpin."
		"\n"
		"- Not working completely yet -\n"
		"After adding this control list to a window it is not possible to change\n"
		"the settings of this spin control.");

	PyObject* ControlList_GetSpinControl(ControlTextBox *self, PyObject *args)
	{
		Py_INCREF(self->pControlSpin);
		return (PyObject*)self->pControlSpin;
	}

	PyDoc_STRVAR(setImageDimensions__doc__,
		"setImageDimensions(int width, int height) -- ");

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

	PyDoc_STRVAR(setItemHeight__doc__,
		"setItemHeight(int height) -- ");

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

	PyDoc_STRVAR(setSpace__doc__,
		"setSpace(int space) -- Set's the space between ListItems");

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
	
	PyDoc_STRVAR(getSelectedPosition__doc__,
		"getSelectedPosition() -- Returns the position of the selected item.\n"
		"\n"
		"Position will be returned as an int");

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

	PyDoc_STRVAR(getSelectedItem__doc__,
		"getSelectedPosition() -- Returns the selected ListItem.\n"
		"\n"
		"Same as getSelectedPosition(), but instead of an int a ListItem is returned.\n"
		"See windowexample.py on how to use this.");

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
		{"addItem", (PyCFunction)ControlList_AddItem, METH_VARARGS, addItem__doc__},
		{"reset", (PyCFunction)ControlList_Reset, METH_VARARGS, reset__doc__},
		{"getSpinControl", (PyCFunction)ControlList_GetSpinControl, METH_VARARGS, getSpinControl__doc__},
		{"getSelectedPosition", (PyCFunction)ControlList_GetSelectedPosition, METH_VARARGS, getSelectedPosition__doc__},
		{"getSelectedItem", (PyCFunction)ControlList_GetSelectedItem, METH_VARARGS, getSelectedItem__doc__},
		{"setImageDimensions", (PyCFunction)ControlList_SetImageDimensions, METH_VARARGS, setImageDimensions__doc__},
		{"setItemHeight", (PyCFunction)ControlList_SetItemHeight, METH_VARARGS, setItemHeight__doc__},
		{"setSpace", (PyCFunction)ControlList_SetSpace, METH_VARARGS, setSpace__doc__},
		{NULL, NULL, 0, NULL}
	};

	PyDoc_STRVAR(controlList__doc__,
		"ControlList class.\n"
		"\n"
		"ControlList(int x, int y, int width, int height[, buttonTexture, buttonFocusTexture])");

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
			controlList__doc__,        /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			ControlList_methods,       /* tp_methods */
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
