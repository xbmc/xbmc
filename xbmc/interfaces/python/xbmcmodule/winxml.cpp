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

#include "winxml.h"
#include "pyutil.h"
#include "GUIPythonWindowXML.h"
#include "addons/Skin.h"
#include "utils/URIUtils.h"
#include "filesystem/File.h"

using namespace std;
using namespace ADDON;

#define ACTIVE_WINDOW g_windowManager.GetActiveWindow()


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

    new(&self->sXMLFileName) string();
    new(&self->sFallBackPath) string();
    new(&self->vecControls) std::vector<Control*>();

    self->iWindowId = -1;
    PyObject* pyOXMLname = NULL;
    PyObject* pyOname = NULL;
    PyObject* pyDName = NULL;
    PyObject* pyRes = NULL;

    string strXMLname, strFallbackPath;
    string strDefault = "Default";
    string resolution = "720p";

    if (!PyArg_ParseTuple(args, (char*)"OO|OO", &pyOXMLname, &pyOname, &pyDName, &pyRes)) return NULL;

    PyXBMCGetUnicodeString(strXMLname, pyOXMLname);
    PyXBMCGetUnicodeString(strFallbackPath, pyOname);
    if (pyDName) PyXBMCGetUnicodeString(strDefault, pyDName);
    if (pyRes) PyXBMCGetUnicodeString(resolution, pyRes);

    // Check to see if the XML file exists in current skin. If not use fallback path to find a skin for the script
    RESOLUTION_INFO res;
    CStdString strSkinPath = g_SkinInfo->GetSkinPath(strXMLname, &res);

    if (!XFILE::CFile::Exists(strSkinPath))
    {
      // Check for the matching folder for the skin in the fallback skins folder
      CStdString fallbackPath = URIUtils::AddFileToFolder(strFallbackPath, "resources");
      fallbackPath = URIUtils::AddFileToFolder(fallbackPath, "skins");
      CStdString basePath = URIUtils::AddFileToFolder(fallbackPath, g_SkinInfo->ID());
      strSkinPath = g_SkinInfo->GetSkinPath(strXMLname, &res, basePath);
      if (!XFILE::CFile::Exists(strSkinPath))
      {
        // Finally fallback to the DefaultSkin as it didn't exist in either the XBMC Skin folder or the fallback skin folder
        CStdString str("none");
        AddonProps props(str, ADDON_SKIN, "", "");
        props.path = URIUtils::AddFileToFolder(fallbackPath, strDefault);
        CSkinInfo::TranslateResolution(resolution, res);
        CSkinInfo skinInfo(props, res);

        skinInfo.Start();
        strSkinPath = skinInfo.GetSkinPath(strXMLname, &res);
        if (!XFILE::CFile::Exists(strSkinPath))
        {
          PyErr_SetString(PyExc_TypeError, "XML File for Window is missing");
          return NULL;
        }
      }
    }

    self->sFallBackPath = strFallbackPath;
    self->sXMLFileName = strSkinPath;
    self->bUsingXML = true;

    // create new GUIWindow
    if (!Window_CreateNewWindow((Window*)self, false))
    {
      // error is already set by Window_CreateNewWindow, just release the memory
      self->vecControls.clear();
      self->vecControls.~vector();
      self->sFallBackPath.~string();
      self->sXMLFileName.~string();
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
    if (!PyArg_ParseTuple(args, (char*)"i", &itemPosition)) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    // Tells the window to remove the item at the specified position from the FileItem vector
    PyXBMCGUILock();
    pwx->RemoveItem(itemPosition);
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // addItem() method
  PyDoc_STRVAR(addItem__doc__,
    "addItem(item[, position]) -- Add a new item to this Window List.\n"
    "\n"
    "item            : string, unicode or ListItem - item to add.\n"
    "position        : [opt] integer - position of item to add. (NO Int = Adds to bottom,0 adds to top, 1 adds to one below from top,-1 adds to one above from bottom etc etc )\n"
    "                                - If integer positions are greater than list size, negative positions will add to top of list, positive positions will add to bottom of list\n"
    "example:\n"
    "  - self.addItem('Reboot XBMC', 0)\n");

  PyObject* WindowXML_AddItem(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    PyObject *pObject;
    int itemPosition = INT_MAX;
    if (!PyArg_ParseTuple(args, (char*)"O|i", &pObject, &itemPosition)) return NULL;

    string strText;
    ListItem* pListItem = NULL;

    if (ListItem_CheckExact(pObject))
    {
      // object is a listitem
      pListItem = (ListItem*)pObject;
      Py_INCREF(pListItem);
    }
    else
    {
      // object is probably a text item
      if (!PyXBMCGetUnicodeString(strText, pObject, 1)) return NULL;
      // object is a unicode string now, create a new ListItem
      pListItem = ListItem_FromString(strText);
    }

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    // Tells the window to add the item to FileItem vector
    PyXBMCGUILock();
    pwx->AddItem(pListItem->item, itemPosition);
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // refreshList() method
  /*PyDoc_STRVAR(refreshList__doc__,
    "refreshList() -- Updates this Window List. (any new items will be shown once this command is ran.\n"
    "\n"
    "example:\n"
    "  - self.refrestList()\n");*/

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

    PyXBMCGUILock();
    pwx->ClearList();
    PyXBMCGUIUnlock();

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
    if (!PyArg_ParseTuple(args, (char*)"i", &listPos)) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyXBMCGUILock();
    pwx->SetCurrentListPosition(listPos);
    PyXBMCGUIUnlock();

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

    PyXBMCGUILock();
    int listPos = pwx->GetCurrentListPosition();
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_BuildValue((char*)"l", listPos);
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
    if (!PyArg_ParseTuple(args, (char*)"i", &listPos)) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyXBMCGUILock();
    CFileItemPtr fi = pwx->GetListItem(listPos);

    if (fi == NULL)
    {
      PyXBMCGUIUnlock();
      PyErr_SetString(PyExc_TypeError, "Index out of range");
      return NULL;
    }

    ListItem* sListItem = (ListItem*)ListItem_Type.tp_alloc(&ListItem_Type, 0);
    sListItem->item = fi;
    PyXBMCGUIUnlock();

    Py_INCREF(sListItem);
    return (PyObject *)sListItem;
  }

  // getListSize() method
  PyDoc_STRVAR(getListSize__doc__,
    "getListSize() -- Returns the number of items in this Window List.\n"
    "\n"
    "example:\n"
    "  - listSize = self.getListSize()\n");

  PyObject* WindowXML_GetListSize(WindowXML *self, PyObject *args)
  {
    if (!self->pWindow) return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;

    PyXBMCGUILock();
    int listSize = pwx->GetListSize();
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_BuildValue((char*)"l", listSize);
  }

  // setProperty() method
  PyDoc_STRVAR(setProperty__doc__,
    "setProperty(key, value) -- Sets a container property, similar to an infolabel.\n"
    "\n"
    "key            : string - property name.\n"
    "value          : string or unicode - value of property.\n"
    "\n"
    "*Note, Key is NOT case sensitive.\n"
    "       You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - self.setProperty('Category', 'Newest')\n");

  PyObject* WindowXML_SetProperty(WindowXML *self, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = { "key", "value", NULL };
    char *key = NULL;
    PyObject *value = NULL;

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"sO",
      (char**)keywords,
      &key,
      &value))
    {
      return NULL;
    }
    if (!key || !value) return NULL;

    CStdString uText;
    if (!PyXBMCGetUnicodeString(uText, value, 1))
      return NULL;

    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;
    CStdString lowerKey = key;

    PyXBMCGUILock();
    pwx->SetProperty(lowerKey.ToLower(), uText.c_str());
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(windowXML__doc__,
    "WindowXML class.\n"
    "\n"
    "WindowXML(self, xmlFilename, scriptPath[, defaultSkin, defaultRes]) -- Create a new WindowXML script.\n"
    "\n"
    "xmlFilename     : string - the name of the xml file to look for.\n"
    "scriptPath      : string - path to script. used to fallback to if the xml doesn't exist in the current skin. (eg os.getcwd())\n"
    "defaultSkin     : [opt] string - name of the folder in the skins path to look in for the xml. (default='Default')\n"
    "defaultRes      : [opt] string - default skins resolution. (default='720p')\n"
    "\n"
    "*Note, skin folder structure is eg(resources/skins/Default/720p)\n"
    "\n"
    "example:\n"
    " - ui = GUI('script-Lyrics-main.xml', os.getcwd(), 'LCARS', 'PAL')\n"
    "   ui.doModal()\n"
    "   del ui\n");

  PyMethodDef WindowXML_methods[] = {
    {(char*)"addItem", (PyCFunction)WindowXML_AddItem, METH_VARARGS, addItem__doc__},
    {(char*)"removeItem", (PyCFunction)WindowXML_RemoveItem, METH_VARARGS, removeItem__doc__},
    {(char*)"getCurrentListPosition", (PyCFunction)WindowXML_GetCurrentListPosition, METH_VARARGS, getCurrentListPosition__doc__},
    {(char*)"setCurrentListPosition", (PyCFunction)WindowXML_SetCurrentListPosition, METH_VARARGS, setCurrentListPosition__doc__},
    {(char*)"getListItem", (PyCFunction)WindowXML_GetListItem, METH_VARARGS, getListItem__doc__},
    {(char*)"getListSize", (PyCFunction)WindowXML_GetListSize, METH_VARARGS, getListSize__doc__},
    {(char*)"clearList", (PyCFunction)WindowXML_ClearList, METH_VARARGS, clearList__doc__},
    {(char*)"setProperty", (PyCFunction)WindowXML_SetProperty, METH_VARARGS|METH_KEYWORDS, setProperty__doc__},
    {NULL, NULL, 0, NULL}
  };
// Restore code and data sections to normal.

  PyTypeObject WindowXML_Type;

  void initWindowXML_Type()
  {
    PyXBMCInitializeTypeObject(&WindowXML_Type);

    WindowXML_Type.tp_name = (char*)"xbmcgui.WindowXML";
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


