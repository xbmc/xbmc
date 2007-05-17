#include "stdafx.h"
#include "winxml.h"
#include "../python/python.h"
#include "pyutil.h"
#include "GUIPythonWindowXML.h"
#include "../../../application.h"
#include "../../../../guilib/SkinInfo.h"
#include "../../../Util.h"

#define ACTIVE_WINDOW m_gWindowManager.GetActiveWindow()

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  PyObject* WindowXML_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    WindowXML *self;

    self = (WindowXML*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    self->iWindowId = -1;
    PyObject* pyOXMLname, * pyOname;
    PyObject * pyDName = NULL;
    bool bForceDefaultSkin = false;

    string strXMLname, strFallbackPath;
    string strDefault = "Default";

    if (!PyArg_ParseTuple(args, "OO|Ob", &pyOXMLname, &pyOname, &pyDName, &bForceDefaultSkin )) return NULL;
    PyGetUnicodeString(strXMLname, pyOXMLname);
    PyGetUnicodeString(strFallbackPath, pyOname);
    if (pyDName) PyGetUnicodeString(strDefault, pyDName);
    // Check to see if the XML file exists in current skin. If not use fallback path to find a skin for the script
    RESOLUTION res;

    CStdString strSkinPath = g_SkinInfo.GetSkinPath(strXMLname, &res);
    if (!XFILE::CFile::Exists(strSkinPath))
    {
      // Check for the matching folder for the skin in the fallback skins folder
      strSkinPath = g_SkinInfo.GetSkinPath(strXMLname, &res, strFallbackPath + "\\skins\\" + CUtil::GetFileName(g_SkinInfo.GetBaseDir()));
      if (!XFILE::CFile::Exists(strSkinPath))
      {
        // Finally fallback to the DefaultSkin as it didn't exist in either the XBMC Skin folder or the fallback skin folder
        bForceDefaultSkin = true;
      }
      strXMLname = strSkinPath;
    }

    if (bForceDefaultSkin)
    {
      bForceDefaultSkin = true;
      PyGetUnicodeString(strXMLname, pyOXMLname);
      strSkinPath = g_SkinInfo.GetSkinPath(strXMLname, &res, strFallbackPath + "\\skins\\" + strDefault);

      if (!XFILE::CFile::Exists(strSkinPath))
      {
        strSkinPath = strFallbackPath + "\\skins\\" + strDefault + "\\pal\\" + strXMLname;
        res = PAL_4x3;
        if (!XFILE::CFile::Exists(strSkinPath))
        {
          PyErr_SetString(PyExc_TypeError, "XML File for Window is missing");
          return NULL;
        }
      }
      strXMLname = strSkinPath;
    }

    self->sFallBackPath = strFallbackPath;
    self->sXMLFileName = strXMLname;
    self->bUsingXML = true;
    // create new GUIWindow
    if (!Window_CreateNewWindow((Window*)self, false))
    {
      // error is already set by Window_CreateNewWindow, just release the memory
      self->ob_type->tp_free((PyObject*)self);
      return NULL;
    }
    ((CGUIWindow*)(self->pWindow))->SetCoordsRes(res);
    return (PyObject*)self;
  }

  // removeItem() method
  PyDoc_STRVAR(removeItem__doc__,
    "removeItem(position) -- Removes a specified item based on position, from the Window List.\n"
    "\n"
    "position        : integer - position of item to remove.\n"
    "\n"
    "example:\n"
    "  - self.removeItem(5)\n");

  PyObject* WindowXML_RemoveItem(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    int itemPosition;
    if (!PyArg_ParseTuple(args, "i", &itemPosition)) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    // Tells the window to remove the item at the specified position from the FileItem vector
    PyGUILock();
    pwx->RemoveItem(itemPosition);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // addItem() method
  PyDoc_STRVAR(addItem__doc__,
    "addItem(item[, position]) -- Add a new item to this Window List.\n"
    "\n"
    "item            : string, unicode or ListItem - item to add.\n"
    "position        : [opt] integer - position of item to add. (0 adds to top, -1 adds to end[default])\n"
    "\n"
    "example:\n"
    "  - self.addItem('Reboot XBMC', 0)\n");

  PyObject* WindowXML_AddItem(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    PyObject *pObject;
    int itemPosition = -1;
    if (!PyArg_ParseTuple(args, "O|i", &pObject, &itemPosition)) return NULL;

    string strText;
    ListItem* pListItem = NULL;
    bool bRefresh = true;

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

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    // Tells the window to add the item to FileItem vector
    PyGUILock();
    pwx->AddItem((CFileItem *)pListItem->item, itemPosition);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // refreshList() method
  PyDoc_STRVAR(refreshList__doc__,
    "refreshList() -- Updates this Window List. (any new items will be shown once this command is ran.\n"
    "\n"
    "example:\n"
    "  - self.refrestList()\n");

  PyObject* WindowXML_RefreshList(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyGUILock();
    pwx->RefreshList();
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // clearList() method
  PyDoc_STRVAR(clearList__doc__,
    "clearList() -- Clear the Window List.\n"
    "\n"
    "example:\n"
    "  - self.clearList()\n");

  PyObject* WindowXML_ClearList(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyGUILock();
    pwx->ClearList();
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // setCurrentListPosition() method
  PyDoc_STRVAR(setCurrentListPosition__doc__,
    "setCurrentListPosition(position) -- Set the current position in the Window List.\n"
    "\n"
    "position        : integer - position of item to set.\n"
    "\n"
    "example:\n"
    "  - self.setCurrentListPosition(5)\n");

  PyObject* WindowXML_SetCurrentListPosition(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    int listPos = -1;
    if (!PyArg_ParseTuple(args, "i", &listPos)) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyGUILock();
    pwx->SetCurrentListPosition(listPos);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // getCurrentListPosition() method
  PyDoc_STRVAR(getCurrentListPosition__doc__,
    "getCurrentListPosition() -- Gets the current position in the Window List.\n"
    "\n"
    "example:\n"
    "  - pos = self.getCurrentListPosition()\n");

  PyObject* WindowXML_GetCurrentListPosition(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyGUILock();
    int listPos = pwx->GetCurrentListPosition();
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_BuildValue("l", listPos);
  }

  // getListItem() method
  PyDoc_STRVAR(getListItem__doc__,
    "getListItem(position) -- Returns a given ListItem in this Window List.\n"
    "\n"
    "position        : integer - position of item to return.\n"
    "\n"
    "example:\n"
    "  - listitem = self.getListItem(6)\n");

  PyObject* WindowXML_GetListItem(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    int listPos = -1;
    if (!PyArg_ParseTuple(args, "i", &listPos)) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyGUILock();
    CFileItem * fi = pwx->GetListItem(listPos);

    if (fi == NULL)
    {
      PyGUIUnlock();
      PyErr_SetString(PyExc_TypeError, "Index out of range");
      return NULL;
    }

    ListItem* sListItem = (ListItem*)ListItem_Type.tp_alloc(&ListItem_Type, 0);
    sListItem->item = new CGUIListItem();
    sListItem->item->SetLabel(fi->GetLabel());
    sListItem->item->SetLabel2(fi->GetLabel2());
    sListItem->item->SetIconImage(fi->GetIconImage());
    sListItem->item->SetThumbnailImage(fi->GetThumbnailImage());
    PyGUIUnlock();

    Py_INCREF(sListItem);
    return (PyObject *)sListItem;
  }

  // getCurrentListItem() method
  PyDoc_STRVAR(getCurrentListItem__doc__,
    "getCurrentListItem() -- Returns the current ListItem in this Window List.\n"
    "\n"
    "example:\n"
    "  - listitem = self.getCurrentListItem()\n");

  PyObject* WindowXML_GetCurrentListItem(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyGUILock();
    CFileItem * fi = pwx->GetCurrentListItem();

    if (fi == NULL)
    {
      PyGUIUnlock();
      PyErr_SetString(PyExc_TypeError, "Index out of range");
      return NULL;
    }

    ListItem* sListItem = (ListItem*)ListItem_Type.tp_alloc(&ListItem_Type, 0);
    sListItem->item = new CGUIListItem();
    sListItem->item->SetLabel(fi->GetLabel());
    sListItem->item->SetLabel2(fi->GetLabel2());
    sListItem->item->SetIconImage(fi->GetIconImage());
    sListItem->item->SetThumbnailImage(fi->GetThumbnailImage());
    PyGUIUnlock();

    Py_INCREF(sListItem);
    return (PyObject *)sListItem;
  }

  // setLabel() method
  PyDoc_STRVAR(setLabel__doc__,
    "setLabel(label) -- Sets the ListItem's label.\n"
    "\n"
    "label           : string or unicode - text string.\n"
    "\n"
    "example:\n"
    "  - self.setLabel('Casino Royale')\n");

  PyObject* WindowXML_SetLabel(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    PyObject* uLine = NULL;
    if (!PyArg_ParseTuple(args, "O", &uLine)) return NULL;

    string utf8Line;
    if (uLine && !PyGetUnicodeString(utf8Line, uLine, 1))
      return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyGUILock();
    pwx->GetCurrentListItem()->SetLabel(utf8Line);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // getLabel() method
  PyDoc_STRVAR(getLabel__doc__,
    "getLabel() -- Returns the ListItem's label.\n"
    "\n"
    "example:\n"
    "  - label = self.getLabel()\n");

  PyObject* WindowXML_GetLabel(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyGUILock();
    CStdString strLabel = pwx->GetCurrentListItem()->GetLabel();
    PyGUIUnlock();

    return Py_BuildValue("s", strLabel.c_str());
  }

  // setLabel2() method
  PyDoc_STRVAR(setLabel2__doc__,
    "setLabel2(label) -- Sets the media listitem's label2.\n"
    "\n"
    "label           : string or unicode - text string.\n"
    "\n"
    "example:\n"
    "  - self.setLabel2('[PG-13]')\n");

  PyObject* WindowXML_SetLabel2(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    PyObject* uLine = NULL;
    if (!PyArg_ParseTuple(args, "O", &uLine)) return NULL;

    string utf8Line;
    if (uLine && !PyGetUnicodeString(utf8Line, uLine, 1))
      return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyGUILock();
    pwx->GetCurrentListItem()->SetLabel2(utf8Line);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // getLabel2() method
  PyDoc_STRVAR(getLabel2__doc__,
    "getLabel2() -- Returns the ListItem's label2.\n"
    "\n"
    "example:\n"
    "  - label = self.getLabel2()\n");

  PyObject* WindowXML_GetLabel2(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyGUILock();
    CStdString strLabel2 = pwx->GetCurrentListItem()->GetLabel2();
    PyGUIUnlock();

    return Py_BuildValue("s", strLabel2.c_str());
  }

  // setThumbnailImage() method
  PyDoc_STRVAR(setThumbnailImage__doc__,
    "setThumbnailImage(thumb) -- Sets the ListItem's thumbnail image.\n"
    "\n"
    "thumb           : string - image filename.\n"
    "\n"
    "example:\n"
    "  - self.setThumbnailImage('emailread.png')\n");

  PyObject* WindowXML_SetThumbnailImage(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    char *cLine = NULL;
    if (!PyArg_ParseTuple(args, "s", &cLine)) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyGUILock();
    pwx->GetCurrentListItem()->SetThumbnailImage(cLine ? cLine : "");
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // getThumbnailImage() method
  PyDoc_STRVAR(getThumbnailImage__doc__,
    "getThumbnailImage() -- Returns the ListItem's thumbnail.\n"
    "\n"
    "example:\n"
    "  - thumb = self.getThumbnailImage()\n");

  PyObject* WindowXML_GetThumbnailImage(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyGUILock();
    CStdString strThumb = pwx->GetCurrentListItem()->GetThumbnailImage();
    PyGUIUnlock();

    return Py_BuildValue("s", strThumb.c_str());
  }

  // setIconImage() method
  PyDoc_STRVAR(setIconImage__doc__,
    "setIconImage(icon) -- Sets the ListItem's icon image.\n"
    "\n"
    "icon            : string - image filename.\n"
    "\n"
    "example:\n"
    "  - self.setIconImage('emailread.png')\n");

  PyObject* WindowXML_SetIconImage(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    char *cLine = NULL;
    if (!PyArg_ParseTuple(args, "s", &cLine)) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyGUILock();
    pwx->GetCurrentListItem()->SetIconImage(cLine ? cLine : "");
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // getIconImage() method
  PyDoc_STRVAR(getIconImage__doc__,
    "getIconImage() -- Returns the ListItem's icon.\n"
    "\n"
    "example:\n"
    "  - thumb = self.getIconImage()\n");

  PyObject* WindowXML_GetIconImage(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyGUILock();
    CStdString strIcon = pwx->GetCurrentListItem()->GetIconImage();
    PyGUIUnlock();

    return Py_BuildValue("s", strIcon.c_str());
  }

  // select() method
  PyDoc_STRVAR(select__doc__,
    "select(selected) -- Sets the ListItem's selected status.\n"
    "\n"
    "selected        : bool - True=selected/False=not selected\n"
    "\n"
    "example:\n"
    "  - self.select(True)\n");

  PyObject* WindowXML_Select(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    bool bOnOff = false;
    if (!PyArg_ParseTuple(args, "b", &bOnOff)) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyGUILock();
    pwx->GetCurrentListItem()->Select(bOnOff);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // isSelected() method
  PyDoc_STRVAR(isSelected__doc__,
    "isSelected() -- Returns the ListItem's selected status.\n"
    "\n"
    "example:\n"
    "  - is = self.isSelected()\n");

  PyObject* WindowXML_IsSelected(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyGUILock();
    bool bOnOff = pwx->GetCurrentListItem()->IsSelected();
    PyGUIUnlock();

    return Py_BuildValue("b", bOnOff);
  }

  PyDoc_STRVAR(windowXML__doc__,
    "WindowXML class.\n"
    "\n"
    "WindowXML(self, XMLname, fallbackPath[, defaultskinname, forceFallback) -- Create a new WindowXML to rendered a xml onto it.\n"
    "\n"
    "XMLname         : string - the name of the xml file to look for.\n"
    "fallbackPath    : string - the directory to fallback to if the xml doesn't exist in the current skin.\n"
    "defaultskinname : [opt] string - name of the folder in the fallback path to look in for the xml. 'Default' is used if this is not set.\n"
    "forceFallback   : [opt] boolean - if true then it will look only in the defaultskinname folder.\n"
    );
  
  PyMethodDef WindowXML_methods[] = {
    {"addItem", (PyCFunction)WindowXML_AddItem, METH_VARARGS, addItem__doc__},
    {"removeItem", (PyCFunction)WindowXML_RemoveItem, METH_VARARGS, removeItem__doc__},
    {"refreshList", (PyCFunction)WindowXML_RefreshList, METH_VARARGS, refreshList__doc__},
    {"getCurrentListPosition", (PyCFunction)WindowXML_GetCurrentListPosition, METH_VARARGS, getCurrentListPosition__doc__},
    {"setCurrentListPosition", (PyCFunction)WindowXML_SetCurrentListPosition, METH_VARARGS, setCurrentListPosition__doc__},
    {"getListItem", (PyCFunction)WindowXML_GetListItem, METH_VARARGS, getListItem__doc__},
    {"getCurrentListItem", (PyCFunction)WindowXML_GetCurrentListItem, METH_VARARGS, getCurrentListItem__doc__},
    {"clearList", (PyCFunction)WindowXML_ClearList, METH_VARARGS, clearList__doc__},
    {"setLabel", (PyCFunction)WindowXML_SetLabel, METH_VARARGS, setLabel__doc__},
    {"getLabel", (PyCFunction)WindowXML_GetLabel, METH_VARARGS, getLabel__doc__},
    {"setLabel2", (PyCFunction)WindowXML_SetLabel2, METH_VARARGS, setLabel2__doc__},
    {"getLabel2", (PyCFunction)WindowXML_GetLabel2, METH_VARARGS, getLabel2__doc__},
    {"setThumbnailImage", (PyCFunction)WindowXML_SetThumbnailImage, METH_VARARGS, setThumbnailImage__doc__},
    {"getThumbnailImage", (PyCFunction)WindowXML_GetThumbnailImage, METH_VARARGS, getThumbnailImage__doc__},
    {"setIconImage", (PyCFunction)WindowXML_SetIconImage, METH_VARARGS, setIconImage__doc__},
    {"getIconImage", (PyCFunction)WindowXML_GetIconImage, METH_VARARGS, getIconImage__doc__},
    {"select", (PyCFunction)WindowXML_Select, METH_VARARGS, select__doc__},
    {"isSelected", (PyCFunction)WindowXML_IsSelected, METH_VARARGS, isSelected__doc__},
    {NULL, NULL, 0, NULL}
  };
// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

  PyTypeObject WindowXML_Type;

  void initWindowXML_Type()
  {
    PyInitializeTypeObject(&WindowXML_Type);

    WindowXML_Type.tp_name = "xbmcgui.WindowXML";
    WindowXML_Type.tp_basicsize = sizeof(WindowXML);
    WindowXML_Type.tp_dealloc = (destructor)Window_Dealloc;
    WindowXML_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    WindowXML_Type.tp_doc = windowXML__doc__;
    WindowXML_Type.tp_methods = WindowXML_methods;
    WindowXML_Type.tp_base = &Window_Type;
    WindowXML_Type.tp_new = WindowXML_New;
  }
}

#ifdef __cplusplus
}
#endif

