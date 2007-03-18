#include "../../../stdafx.h"
#include "winxml.h"
#include "..\python\python.h"
#include "pyutil.h"
#include "GUIPythonWindowXML.h"
#include "..\..\..\application.h"
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
    //bool
    string strXMLname, strFallbackPath;

    if (!PyArg_ParseTuple(args, "OO", &pyOXMLname, &pyOname)) return NULL;
    PyGetUnicodeString(strXMLname, pyOXMLname);
    PyGetUnicodeString(strFallbackPath, pyOname);

    // Check to see if the XML file exists in current skin. If not use fallback path to find a skin for the script
    RESOLUTION res;
    CStdString strSkinPath = g_SkinInfo.GetSkinPath(strXMLname,&res);
    if (!XFILE::CFile::Exists(strSkinPath))
    {
      strSkinPath = g_SkinInfo.GetSkinPath(strXMLname,&res,strFallbackPath);
      if (!XFILE::CFile::Exists(strSkinPath))
      {
        strSkinPath = strFallbackPath + "\\pal\\" + strXMLname;
        res = PAL_4x3;
      }

      strXMLname = strSkinPath; // We would do this if we didn't copy the xml to the skin
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
  /*
   * WindowXML_AddItem
   * (string label) / (ListItem)
   * ListItem is added to vector
   * For a string we create a new ListItem and add it to the vector
   */
  PyDoc_STRVAR(addItem__doc__,
    "addItem(item[,refreshList]) -- Add a new item to this window list.\n"
    "\n"
    "item               : string, unicode or ListItem - item to add.\n"
    "refreshList        : [optional] true - refreshes the gui/list after add\n"
    "\n"
    "example:\n"
    "  - self.addItem('Reboot XBMC',true)\n");

  PyObject* WindowXML_AddItem(WindowXML *self, PyObject *args)
  {
    PyObject *pObject;
    string strText;
    ListItem* pListItem = NULL;
    bool bRefresh = true;
    if (!PyArg_ParseTuple(args, "O|b", &pObject,&bRefresh))  return NULL;
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
    pwx->AddItem((CFileItem *)pListItem->item,bRefresh);

    // create message
    //CGUIMessage msg(GUI_MSG_LABEL_ADD, self->iWindowId, self->iWindowId);
    //msg.SetLPVOID(pListItem->item);

    // send message
    PyGUILock();
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
    //bool bRefresh = true;
    //if (!PyArg_ParseTuple(args, "|b",&bRefresh))  return NULL;
    CGUIPythonWindowXML * pwx = (CGUIPythonWindowXML*)self->pWindow;
    pwx->ClearList();
    Py_INCREF(Py_None);
    return Py_None;
  }
  PyDoc_STRVAR(windowXML__doc__,
    "WindowXML class.\n");
  PyMethodDef WindowXML_methods[] = {
    {"addItem", (PyCFunction)WindowXML_AddItem, METH_VARARGS, addItem__doc__},
    {"refreshList", (PyCFunction)WindowXML_RefreshList, METH_VARARGS, RefreshList__doc__},
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

