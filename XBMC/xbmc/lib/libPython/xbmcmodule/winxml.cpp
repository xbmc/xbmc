#include "stdafx.h"
#include "winxml.h"
#include "../python/Python.h"
#include "pyutil.h"
#include "GUIPythonWindowXML.h"
#include "../../../Application.h"
#include "../../../../guilib/SkinInfo.h"
#include "../../../Util.h"

#define ACTIVE_WINDOW  m_gWindowManager.GetActiveWindow()

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
    if (pyDName)  PyGetUnicodeString(strDefault, pyDName);
    // Check to see if the XML file exists in current skin. If not use fallback path to find a skin for the script
    RESOLUTION res;
    
    CStdString strSkinPath = g_SkinInfo.GetSkinPath(strXMLname,&res);
    if (!XFILE::CFile::Exists(strSkinPath))
    {
      // Check for the matching folder for the skin in the fallback skins folder
      CStdString basePath;
      CUtil::AddFileToFolder(strFallbackPath, "skins", basePath);
      CUtil::AddFileToFolder(basePath, CUtil::GetFileName(g_SkinInfo.GetBaseDir()), basePath);
      strSkinPath = g_SkinInfo.GetSkinPath(strXMLname,&res,basePath);
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
      strSkinPath = g_SkinInfo.GetSkinPath(strXMLname,&res,strFallbackPath + "\\skins\\" + strDefault);
      
      if (!XFILE::CFile::Exists(strSkinPath))
      {
        strSkinPath = strFallbackPath + "\\skins\\"+ strDefault + "\\pal\\" + strXMLname;
        res = PAL_4x3;
        if (!XFILE::CFile::Exists(strSkinPath))
        {
          PyErr_SetString(PyExc_TypeError, "XML File for Window is missing");
          return NULL;
        }
      }
      strXMLname = strSkinPath;
    }
    
    self->sFallBackPath  = strFallbackPath;
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

  PyDoc_STRVAR(removeItem__doc__,
    "removeItem(itemPosition) -- Removes a specified item based on position in the list from the window list.\n"
    "\n"
    "itemPosition		: interger - position of the item to remove from the window list\n"
    "\n"
    "example:\n"
    "  - self.removeitem(5)\n");

  PyObject* WindowXML_RemoveItem(WindowXML *self, PyObject *args)
  {
    int itemPosition;
    if (!PyArg_ParseTuple(args, "i", &itemPosition))  return NULL;
    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;
    // Tells the window to remove the item at the specified position from the FileItem vector
    pwx->RemoveItem(itemPosition);

    // send message
    PyGUILock();

    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }
  
  PyDoc_STRVAR(addItem__doc__,
    "addItem(item[,itemPosition]) -- Add a new item to this window list.\n"
    "\n"
    "item               : string, unicode or ListItem - item to add.\n"
    "itemPosition       : integer - item position of item to add. (0 adds to top, -1 adds to end).\n"
    "\n"
    "example:\n"
    "  - self.addItem('Reboot XBMC')\n");

  PyObject* WindowXML_AddItem(WindowXML *self, PyObject *args)
  {
    int itemPosition = -1;
    PyObject *pObject;
    string strText;
    ListItem* pListItem = NULL;
    bool bRefresh = true;
    if (!PyArg_ParseTuple(args, "O|i", &pObject,&itemPosition))  return NULL;
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
    
    PyGUILock();
    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;
    
    // Tells the window to add the item to FileItem vector
    pwx->AddItem((CFileItem *)pListItem->item,itemPosition);

    // create message
    //CGUIMessage msg(GUI_MSG_LABEL_ADD, self->iWindowId, self->iWindowId);
    //msg.SetLPVOID(pListItem->item);

    // send message
    
    //if (self->pWindow) self->pWindow->OnMessage(msg);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }
  PyDoc_STRVAR(RefreshList__doc__,
    "refreshList() -- Updates the Windows Lists (any new items will be shown once this command is ran.\n"
    "\n"
    "example:\n"
    "  - self.refrestList()\n");

  PyObject* WindowXML_RefreshList(WindowXML *self, PyObject *args)
  {
    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;
    // Tells the window to add the item to FileItem vector
    pwx->RefreshList();
    Py_INCREF(Py_None);
    return Py_None;
  }
  PyDoc_STRVAR(ClearList__doc__,
    "clearList() -- Clear the Window List\n"
    "\n"
    "example:\n"
    "  - self.clearList()\n");

  PyObject* WindowXML_ClearList(WindowXML *self, PyObject *args)
  {
    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;
    pwx->ClearList();
    Py_INCREF(Py_None);
    return Py_None;
  }
  PyDoc_STRVAR(setCurrentListPosition__doc__,
    "setCurrentListPosition() -- Set the current position of the Window List\n"
    "\n"
    "example:\n"
    "  - self.setCurrentListPosition(5)\n");

  PyObject* WindowXML_SetCurrentListPosition(WindowXML *self, PyObject *args)
  {
    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;
    int listPos = -1;
    if (!PyArg_ParseTuple(args, "i",&listPos))  return NULL;
    pwx->SetCurrentListPosition(listPos);
    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(getCurrentListPosition__doc__,
    "getCurrentListPosition() -- Gets the current position in the Window List\n"
    "\n"
    "example:\n"
    "  - self.getCurrentListPosition()\n");
  
 PyObject* WindowXML_GetCurrentListPosition(WindowXML *self, PyObject *args)
  {
    int listPos = -1;
    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;
    listPos = pwx->GetCurrentListPosition();
    Py_INCREF(Py_None);
    return Py_BuildValue("l", listPos);
  }
  PyDoc_STRVAR(getListItem__doc__,
    "getListItem(int) -- Returns a given ListItem in the WindowList\n"
    "\n"
    "example:\n"
    "  - self.getListItem(6)\n");
  
 PyObject* WindowXML_GetListItem(WindowXML *self, PyObject *args)
  {
    int listPos = -1;
    if (!PyArg_ParseTuple(args, "i",&listPos))  return NULL;
    
    PyGUILock();
    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;
    CFileItem * fi = pwx->GetListItem(listPos);
    
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
  
  PyDoc_STRVAR(windowXML__doc__,
    "WindowXML class.\n"
    "\n"
    "WindowXML(self, XMLname, fallbackPath[, defaultskinname, forceFallback) -- Create a new WindowXML to rendered a xml onto it.\n"
    "\n"
    "XMLname        : string - the name of the xml file to look for.\n"
    "fallbackPath   : string - the directory to fallback to if the xml doesn't exist in the current skin.\n"
    "defaultskinname: [opt] string - name of the folder in the fallback path to look in for the xml. 'Default' is used if this is not set.\n"
    "forceFallback  : [opt] boolean - if true then it will look only in the defaultskinname folder.\n"
    );
  PyMethodDef WindowXML_methods[] = {
    {"addItem", (PyCFunction)WindowXML_AddItem, METH_VARARGS, addItem__doc__},
	{"removeItem", (PyCFunction)WindowXML_RemoveItem, METH_VARARGS, removeItem__doc__},
    {"refreshList", (PyCFunction)WindowXML_RefreshList, METH_VARARGS, RefreshList__doc__},
    {"getCurrentListPosition", (PyCFunction)WindowXML_GetCurrentListPosition, METH_VARARGS,getCurrentListPosition__doc__},
    {"setCurrentListPosition", (PyCFunction)WindowXML_SetCurrentListPosition, METH_VARARGS,setCurrentListPosition__doc__},
    {"getListItem", (PyCFunction)WindowXML_GetListItem, METH_VARARGS,getListItem__doc__},
    {"clearList", (PyCFunction)WindowXML_ClearList, METH_VARARGS, ClearList__doc__},
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

