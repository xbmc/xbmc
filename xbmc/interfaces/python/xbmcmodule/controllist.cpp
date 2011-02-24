/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <Python.h>

#include "../XBPythonDll.h"
#include "guilib/GUIListContainer.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/GUILabel.h"
#include "control.h"
#include "pyutil.h"

using namespace std;

#ifndef __GNUC__
#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")
#endif

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  extern PyObject* ControlSpin_New(void);

  PyObject* ControlList_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = {
      "x", "y", "width", "height", "font",
      "textColor", "buttonTexture", "buttonFocusTexture",
      // maintain order of above items for backward compatibility
      "selectedColor",
      "imageWidth", "imageHeight",
      "itemTextXOffset", "itemTextYOffset",
      "itemHeight", "space", "alignmentY", NULL };//"shadowColor", NULL };
    ControlList *self;
    char *cFont = NULL;
    char *cTextColor = NULL;
    char *cSelectedColor = NULL;
    char *cTextureButton = NULL;
    char *cTextureButtonFocus = NULL;
    //char* cShadowColor = NULL;
    self = (ControlList*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    new(&self->strFont) string();
    new(&self->strTextureButton) string();
    new(&self->strTextureButtonFocus) string();
    new(&self->vecItems) std::vector<PYXBMC::ListItem*>();

    // create a python spin control
    self->pControlSpin = (ControlSpin*)ControlSpin_New();
    if (!self->pControlSpin)
    {
      Py_DECREF( self );
      return NULL;
    }

    // initialize default values
    self->strFont = "font13";
    self->textColor = 0xe0f0f0f0;
    self->selectedColor = 0xffffffff;
    self->imageHeight = 10;
    self->imageWidth = 10;
    self->itemHeight = 27;
    self->space = 2;
    self->itemTextOffsetX = CONTROL_TEXT_OFFSET_X;
    self->itemTextOffsetY = CONTROL_TEXT_OFFSET_Y;
    self->alignmentY = XBFONT_CENTER_Y;
    //self->shadowColor = NULL;

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"llll|ssssslllllll",//s",
      (char**)keywords,
      &self->dwPosX,
      &self->dwPosY,
      &self->dwWidth,
      &self->dwHeight,
      &cFont,
      &cTextColor,
      &cTextureButton,
      &cTextureButtonFocus,
      &cSelectedColor,
      &self->imageWidth,
      &self->imageHeight,
      &self->itemTextOffsetX,
      &self->itemTextOffsetY,
      &self->itemHeight,
      &self->space,
      &self->alignmentY//,
      ))//&cShadowColor))
    {
      Py_DECREF( self );
      return NULL;
    }

    // set specified values
    if (cFont) self->strFont = cFont;
    if (cTextColor)
    {
      sscanf( cTextColor, "%x", &self->textColor );
    }
    if (cSelectedColor)
    {
      sscanf( cSelectedColor, "%x", &self->selectedColor );
    }
    //if (cShadowColor) sscanf( cShadowColor, "%x", &self->shadowColor );

    self->strTextureButton = cTextureButton ? cTextureButton :
      PyXBMCGetDefaultImage((char*)"listcontrol", (char*)"texturenofocus", (char*)"list-nofocus.png");
    self->strTextureButtonFocus = cTextureButtonFocus ? cTextureButtonFocus :
      PyXBMCGetDefaultImage((char*)"listcontrol", (char*)"texturefocus", (char*)"list-focus.png");

    // default values for spin control
    self->pControlSpin->dwPosX = self->dwWidth - 35;
    self->pControlSpin->dwPosY = self->dwHeight - 15;

    return (PyObject*)self;
  }

  void ControlList_Dealloc(ControlList* self)
  {
    // conditionally delete spincontrol
    Py_XDECREF(self->pControlSpin);

    // delete all ListItem from vector
    vector<ListItem*>::iterator it = self->vecItems.begin();
    while (it != self->vecItems.end())
    {
      ListItem* pListItem = *it;
      Py_DECREF(pListItem);
      ++it;
    }
    self->vecItems.clear();
    self->vecItems.~vector();

    self->strFont.~string();
    self->strTextureButton.~string();
    self->strTextureButtonFocus.~string();

    self->ob_type->tp_free((PyObject*)self);
  }

  CGUIControl* ControlList_Create(ControlList* pControl)
  {
    CLabelInfo label;
    label.align = pControl->alignmentY;
    label.font = g_fontManager.GetFont(pControl->strFont);
    label.textColor = label.focusedColor = pControl->textColor;
    //label.shadowColor = pControl->shadowColor;
    label.selectedColor = pControl->selectedColor;
    label.offsetX = (float)pControl->itemTextOffsetX;
    label.offsetY = (float)pControl->itemTextOffsetY;
    // Second label should have the same font, alignment, and colours as the first, but
    // the offsets should be 0.
    CLabelInfo label2 = label;
    label2.offsetX = label2.offsetY = 0;
    label2.align |= XBFONT_RIGHT;

    pControl->pGUIControl = new CGUIListContainer(
      pControl->iParentId,
      pControl->iControlId,
      (float)pControl->dwPosX,
      (float)pControl->dwPosY,
      (float)pControl->dwWidth,
      (float)pControl->dwHeight - pControl->pControlSpin->dwHeight - 5,
      label, label2,
      (CStdString)pControl->strTextureButton,
      (CStdString)pControl->strTextureButtonFocus,
      (float)pControl->itemHeight,
      (float)pControl->imageWidth, (float)pControl->imageHeight,
      (float)pControl->space);

    return pControl->pGUIControl;
  }

  /*
   * ControlList_AddItem
   * (string label) / (ListItem)
   * ListItem is added to vector
   * For a string we create a new ListItem and add it to the vector
   */
PyDoc_STRVAR(addItem__doc__,
    "addItem(item) -- Add a new item to this list control.\n"
    "\n"
    "item               : string, unicode or ListItem - item to add.\n"
    "\n"
    "example:\n"
    "  - cList.addItem('Reboot XBMC')\n");

  PyObject* ControlList_AddItem(ControlList *self, PyObject *args)
  {
    PyObject *pObject;
    if (!PyArg_ParseTuple(args, (char*)"O", &pObject))  return NULL;

    ListItem* pListItem = NULL;
    if (ListItem_CheckExact(pObject))
    {
      // object is a listitem
      pListItem = (ListItem*)pObject;
      Py_INCREF(pListItem);
    }
    else
    {
      string strText;
      // object is probably a text item
      if (!PyXBMCGetUnicodeString(strText, pObject, 1)) return NULL;
      // object is a unicode string now, create a new ListItem
      pListItem = ListItem_FromString(strText);
    }

    // add item to objects vector
    self->vecItems.push_back(pListItem);

    // construct a CFileItemList to pass 'em on to the list
    CGUIListItemPtr items(new CFileItemList());
    for (unsigned int i = 0; i < self->vecItems.size(); i++)
      ((CFileItemList*)items.get())->Add(self->vecItems[i]->item);

    CGUIMessage msg(GUI_MSG_LABEL_BIND, self->iParentId, self->iControlId, 0, 0, items);
    msg.SetPointer(items.get());
    g_windowManager.SendThreadMessage(msg, self->iParentId);

    Py_INCREF(Py_None);
    return Py_None;
  }

PyDoc_STRVAR(addItems__doc__,
    "addItems(items) -- Adds a list of listitems or strings to this list control.\n"
    "\n"
    "items                : List - list of strings, unicode objects or ListItems to add.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments.\n"
    "\n"
    "       Large lists benefit considerably, than using the standard addItem()"
    "\n"
    "example:\n"
    "  - cList.addItems(items=listitems)\n");

  PyObject* ControlList_AddItems(ControlList *self, PyObject *args, PyObject *kwds)
  {
    PyObject *pList = NULL;
    static const char *keywords[] = { "items", NULL };

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"O",
      (char**)keywords,
      &pList) || pList == NULL || !PyObject_TypeCheck(pList, &PyList_Type))
    {
      PyErr_SetString(PyExc_TypeError, "Object should be of type List");
      return NULL;
    }

    CGUIListItemPtr items(new CFileItemList());
    for (int item = 0; item < PyList_Size(pList); item++)
    {
      PyObject *pItem = PyList_GetItem(pList, item);

      ListItem* pListItem = NULL;
      if (ListItem_CheckExact(pItem))
      {
        // object is a listitem
        pListItem = (ListItem*)pItem;
        Py_INCREF(pListItem);
      }
      else
      {
        string strText;
        // object is probably a text item
        if (!PyXBMCGetUnicodeString(strText, pItem, 1)) return NULL;
        // object is a unicode string now, create a new ListItem
        pListItem = ListItem_FromString(strText);
      }

      // add item to objects vector
      self->vecItems.push_back(pListItem);
      ((CFileItemList*)items.get())->Add(pListItem->item);
    }

    // create message
    CGUIMessage msg(GUI_MSG_LABEL_BIND, self->iParentId, self->iControlId, 0, 0, items);
    msg.SetPointer(items.get());
    g_windowManager.SendThreadMessage(msg, self->iParentId);

    Py_INCREF(Py_None);
    return Py_None;
  }

  /*
  * ControlList_SelectItem(int item)
  * Select an item by index
  */
  PyDoc_STRVAR(selectItem,
    "selectItem(item) -- Select an item by index number.\n"
    "\n"
    "item               : integer - index number of the item to select.\n"
    "\n"
    "example:\n"
    "  - cList.selectItem(12)\n");

  PyObject* ControlList_SelectItem(ControlList *self, PyObject *args)
  {
    long itemIndex;

    if (!PyArg_ParseTuple(args, (char*)"l", &itemIndex)) return NULL;

    // create message
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, self->iParentId, self->iControlId, itemIndex);

    // send message
    g_windowManager.SendThreadMessage(msg, self->iParentId);

    Py_INCREF(Py_None);
    return Py_None;
  }

  // reset() method
  PyDoc_STRVAR(reset__doc__,
    "reset() -- Clear all ListItems in this control list.\n"
    "\n"
    "example:\n"
    "  - cList.reset()\n");

  PyObject* ControlList_Reset(ControlList *self, PyObject *args)
  {
    // create message
    ControlList *pControl = (ControlList*)self;
    CGUIMessage msg(GUI_MSG_LABEL_RESET, pControl->iParentId, pControl->iControlId);

    g_windowManager.SendThreadMessage(msg, pControl->iParentId);

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

  // getSpinControl() method
  PyDoc_STRVAR(getSpinControl__doc__,
    "getSpinControl() -- returns the associated ControlSpin object.\n"
    "\n"
    "*Note, Not working completely yet -\n"
    "       After adding this control list to a window it is not possible to change\n"
    "       the settings of this spin control.\n"
    "\n"
    "example:\n"
    "  - ctl = cList.getSpinControl()\n");

  PyObject* ControlList_GetSpinControl(ControlList *self, PyObject *args)
  {
    Py_INCREF(self->pControlSpin);
    return (PyObject*)self->pControlSpin;
  }

  // setImageDimensions() method
  PyDoc_STRVAR(setImageDimensions__doc__,
    "setImageDimensions(imageWidth, imageHeight) -- Sets the width/height of items icon or thumbnail.\n"
    "\n"
    "imageWidth         : [opt] integer - width of items icon or thumbnail.\n"
    "imageHeight        : [opt] integer - height of items icon or thumbnail.\n"
    "\n"
    "example:\n"
    "  - cList.setImageDimensions(18, 18)\n");

  PyObject* ControlList_SetImageDimensions(ControlList *self, PyObject *args)
  {
    if (!PyArg_ParseTuple(args, (char*)"ll", &self->imageWidth, &self->imageHeight))
    {
      return NULL;
    }

    /*
    PyXBMCGUILock();
    if (self->pGUIControl)
    {
      CGUIListControl* pListControl = (CGUIListControl*) self->pGUIControl;
      pListControl->SetImageDimensions((float)self->dwImageWidth, (float)self->dwImageHeight );
    }
    PyXBMCGUIUnlock();
    */
    Py_INCREF(Py_None);
    return Py_None;
  }

  // setItemHeight() method
  PyDoc_STRVAR(setItemHeight__doc__,
    "setItemHeight(itemHeight) -- Sets the height of items.\n"
    "\n"
    "itemHeight         : integer - height of items.\n"
    "\n"
    "example:\n"
    "  - cList.setItemHeight(25)\n");

  PyObject* ControlList_SetItemHeight(ControlList *self, PyObject *args)
  {
    if (!PyArg_ParseTuple(args, (char*)"l", &self->itemHeight)) return NULL;

    /*
    PyXBMCGUILock();
    if (self->pGUIControl)
    {
      CGUIListControl* pListControl = (CGUIListControl*) self->pGUIControl;
      pListControl->SetItemHeight((float)self->dwItemHeight);
    }
    PyXBMCGUIUnlock();
    */
    Py_INCREF(Py_None);
    return Py_None;
  }


  // setPageControlVisible() method
  PyDoc_STRVAR(setPageControlVisible__doc__,
    "setPageControlVisible(visible) -- Sets the spin control's visible/hidden state.\n"
    "\n"
    "visible            : boolean - True=visible / False=hidden.\n"
    "\n"
    "example:\n"
    "  - cList.setPageControlVisible(True)\n");

  PyObject* ControlList_SetPageControlVisible(ControlList *self, PyObject *args)
  {
    char isOn = true;

    if (!PyArg_ParseTuple(args, (char*)"b", &isOn)) return NULL;

    /*
    PyXBMCGUILock();
    if (self->pGUIControl)
    {
      ((CGUIListControl*)self->pGUIControl)->SetPageControlVisible((bool)isOn );
    }
    PyXBMCGUIUnlock();
    */

    Py_INCREF(Py_None);
    return Py_None;
  }

  // setSpace() method
  PyDoc_STRVAR(setSpace__doc__,
    "setSpace(space) -- Set's the space between items.\n"
    "\n"
    "space              : [opt] integer - space between items.\n"
    "\n"
    "example:\n"
    "  - cList.setSpace(5)\n");

  PyObject* ControlList_SetSpace(ControlList *self, PyObject *args)
  {
    if (!PyArg_ParseTuple(args, (char*)"l", &self->space)) return NULL;

    /*
    PyXBMCGUILock();
    if (self->pGUIControl)
    {
      CGUIListControl* pListControl = (CGUIListControl*) self->pGUIControl;
      pListControl->SetSpaceBetweenItems((float)self->dwSpace);
    }
    PyXBMCGUIUnlock();
    */

    Py_INCREF(Py_None);
    return Py_None;
  }

  // getSelectedPosition() method
  PyDoc_STRVAR(getSelectedPosition__doc__,
    "getSelectedPosition() -- Returns the position of the selected item as an integer.\n"
    "\n"
    "*Note, Returns -1 for empty lists.\n"
    "\n"
    "example:\n"
    "  - pos = cList.getSelectedPosition()\n");

  PyObject* ControlList_GetSelectedPosition(ControlList *self, PyObject *args)
  {
    // create message
    ControlList *pControl = (ControlList*)self;
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, pControl->iParentId, pControl->iControlId);
    long pos = -1;

    // send message
    PyXBMCGUILock();
    if ((self->vecItems.size() > 0) && pControl->pGUIControl)
    {
      pControl->pGUIControl->OnMessage(msg);
      pos = msg.GetParam1();
    }
    PyXBMCGUIUnlock();

    return Py_BuildValue((char*)"l", pos);
  }

  // getSelectedItem() method
  PyDoc_STRVAR(getSelectedItem__doc__,
    "getSelectedItem() -- Returns the selected item as a ListItem object.\n"
    "\n"
    "*Note, Same as getSelectedPosition(), but instead of an integer a ListItem object\n"
    "       is returned. Returns None for empty lists.\n"
    "       See windowexample.py on how to use this.\n"
    "\n"
    "example:\n"
    "  - item = cList.getSelectedItem()\n");

  PyObject* ControlList_GetSelectedItem(ControlList *self, PyObject *args)
  {
    // create message
    ControlList *pControl = (ControlList*)self;
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, pControl->iParentId, pControl->iControlId);
    PyObject* pListItem = Py_None;

    // send message
    PyXBMCGUILock();
    if ((self->vecItems.size() > 0) && pControl->pGUIControl)
    {
      pControl->pGUIControl->OnMessage(msg);
      pListItem = (PyObject*)self->vecItems[msg.GetParam1()];
    }
    PyXBMCGUIUnlock();

    Py_INCREF(pListItem);
    return pListItem;
  }

  // size() method
  PyDoc_STRVAR(size__doc__,
    "size() -- Returns the total number of items in this list control as an integer.\n"
    "\n"
    "example:\n"
    "  - cnt = cList.size()\n");

  PyObject* ControlList_Size(ControlList *self)
  {
    return Py_BuildValue((char*)"l", self->vecItems.size());
  }

  // getListItem() method
  PyDoc_STRVAR(getListItem__doc__,
    "getListItem(index) -- Returns a given ListItem in this List.\n"
    "\n"
    "index           : integer - index number of item to return.\n"
    "\n"
    "*Note, throws a ValueError if index is out of range.\n"
    "\n"
    "example:\n"
    "  - listitem = cList.getListItem(6)\n");

  PyObject* ControlList_GetListItem(ControlList *self, PyObject *args)
  {
    int iPos = -1;
    if (!PyArg_ParseTuple(args, (char*)"i", &iPos)) return NULL;

    if (iPos < 0 || iPos >= (int)self->vecItems.size())
    {
      PyErr_SetString(PyExc_ValueError, "Index out of range");
      return NULL;
    }

    PyObject* pListItem = (PyObject*)self->vecItems[iPos];

    Py_INCREF(pListItem);
    return pListItem;
  }

  // getItemHeight() Method
  PyDoc_STRVAR(getItemHeight__doc__,
    "getItemHeight() -- Returns the control's current item height as an integer.\n"
    "\n"
    "example:\n"
    "  - item_height = self.cList.getItemHeight()\n");

  PyObject* ControlList_GetItemHeight(ControlList *self)
  {
    return Py_BuildValue((char*)"l", self->itemHeight);
  }

  // getSpace() Method
  PyDoc_STRVAR(getSpace__doc__,
    "getSpace() -- Returns the control's space between items as an integer.\n"
    "\n"
    "example:\n"
    "  - gap = self.cList.getSpace()\n");

  PyObject* ControlList_GetSpace(ControlList *self)
  {
    return Py_BuildValue((char*)"l", self->space);
  }

PyDoc_STRVAR(setStaticContent__doc__,
    "setStaticContent(items) -- Fills a static list with a list of listitems.\n"
    "\n"
    "items                : List - list of listitems to add.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments.\n"
    "\n"
    "example:\n"
    "  - cList.setStaticContent(items=listitems)\n");

  PyObject* ControlList_SetStaticContent(ControlList *self, PyObject *args, PyObject *kwds)
  {
    PyObject *pList = NULL;
    static const char *keywords[] = { "items", NULL };

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"O",
      (char**)keywords,
      &pList) || pList == NULL || !PyObject_TypeCheck(pList, &PyList_Type))
    {
      PyErr_SetString(PyExc_TypeError, "Object should be of type List");
      return NULL;
    }

    vector<CGUIListItemPtr> items;

    for (int item = 0; item < PyList_Size(pList); item++)
    {
      PyObject *pItem = PyList_GetItem(pList, item);
      if (!ListItem_CheckExact(pItem))
      {
        PyErr_SetString(PyExc_TypeError, "Only ListItems can be passed");
        return NULL;
      }
      // object is a listitem, and we set m_idpeth to 0 as this
      // is used as the visibility condition for the item in the list
      ListItem *listItem = (ListItem*)pItem;
      listItem->item->m_idepth = 0;

      items.push_back((CFileItemPtr &)listItem->item);
    }
    // set static list
    ((CGUIBaseContainer *)self->pGUIControl)->SetStaticContent(items);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef ControlList_methods[] = {
    {(char*)"addItem", (PyCFunction)ControlList_AddItem, METH_VARARGS, addItem__doc__},
    {(char*)"selectItem", (PyCFunction)ControlList_SelectItem, METH_VARARGS,  selectItem},
    {(char*)"reset", (PyCFunction)ControlList_Reset, METH_VARARGS, reset__doc__},
    {(char*)"getSpinControl", (PyCFunction)ControlList_GetSpinControl, METH_VARARGS, getSpinControl__doc__},
    {(char*)"getSelectedPosition", (PyCFunction)ControlList_GetSelectedPosition, METH_VARARGS, getSelectedPosition__doc__},
    {(char*)"getSelectedItem", (PyCFunction)ControlList_GetSelectedItem, METH_VARARGS, getSelectedItem__doc__},
    {(char*)"setImageDimensions", (PyCFunction)ControlList_SetImageDimensions, METH_VARARGS, setImageDimensions__doc__},
    {(char*)"setItemHeight", (PyCFunction)ControlList_SetItemHeight, METH_VARARGS, setItemHeight__doc__},
    {(char*)"setSpace", (PyCFunction)ControlList_SetSpace, METH_VARARGS, setSpace__doc__},
    {(char*)"setPageControlVisible", (PyCFunction)ControlList_SetPageControlVisible, METH_VARARGS, setPageControlVisible__doc__},
    {(char*)"size", (PyCFunction)ControlList_Size, METH_VARARGS, size__doc__},
    {(char*)"getItemHeight", (PyCFunction)ControlList_GetItemHeight, METH_VARARGS, getItemHeight__doc__},
    {(char*)"getSpace", (PyCFunction)ControlList_GetSpace, METH_VARARGS, getSpace__doc__},
    {(char*)"getListItem", (PyCFunction)ControlList_GetListItem, METH_VARARGS, getListItem__doc__},
    {(char*)"setStaticContent", (PyCFunction)ControlList_SetStaticContent, METH_VARARGS|METH_KEYWORDS, setStaticContent__doc__},
    {(char*)"addItems", (PyCFunction)ControlList_AddItems, METH_VARARGS|METH_KEYWORDS, addItems__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(controlList__doc__,
    "ControlList class.\n"
    "\n"
    "ControlList(x, y, width, height[, font, textColor, buttonTexture, buttonFocusTexture,\n"
    "            selectedColor, imageWidth, imageHeight, itemTextXOffset, itemTextYOffset,\n"
    "            itemHeight, space, alignmentY])\n"//, shadowColor])\n"
    "\n"
    "x                  : integer - x coordinate of control.\n"
    "y                  : integer - y coordinate of control.\n"
    "width              : integer - width of control.\n"
    "height             : integer - height of control.\n"
    "font               : [opt] string - font used for items label. (e.g. 'font13')\n"
    "textColor          : [opt] hexstring - color of items label. (e.g. '0xFFFFFFFF')\n"
    "buttonTexture      : [opt] string - filename for focus texture.\n"
    "buttonFocusTexture : [opt] string - filename for no focus texture.\n"
    "selectedColor      : [opt] integer - x offset of label.\n"
    "imageWidth         : [opt] integer - width of items icon or thumbnail.\n"
    "imageHeight        : [opt] integer - height of items icon or thumbnail.\n"
    "itemTextXOffset    : [opt] integer - x offset of items label.\n"
    "itemTextYOffset    : [opt] integer - y offset of items label.\n"
    "itemHeight         : [opt] integer - height of items.\n"
    "space              : [opt] integer - space between items.\n"
    "alignmentY         : [opt] integer - Y-axis alignment of items label - *Note, see xbfont.h\n"
    //"shadowColor        : [opt] hexstring - color of items label's shadow. (e.g. '0xFF000000')\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "       After you create the control, you need to add it to the window with addControl().\n"
    "\n"
    "example:\n"
    "  - self.cList = xbmcgui.ControlList(100, 250, 200, 250, 'font14', space=5)\n"
  );

// Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject ControlList_Type;

  void initControlList_Type()
  {
    PyXBMCInitializeTypeObject(&ControlList_Type);

    ControlList_Type.tp_name = (char*)"xbmcgui.ControlList";
    ControlList_Type.tp_basicsize = sizeof(ControlList);
    ControlList_Type.tp_dealloc = (destructor)ControlList_Dealloc;
    ControlList_Type.tp_compare = 0;
    ControlList_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    ControlList_Type.tp_doc = controlList__doc__;
    ControlList_Type.tp_methods = ControlList_methods;
    ControlList_Type.tp_base = &Control_Type;
    ControlList_Type.tp_new = ControlList_New;
  }
}

#ifdef __cplusplus
}
#endif
