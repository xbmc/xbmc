#include "../../../stdafx.h"
#include "winxml.h"
#include "..\python\python.h"
#include "pyutil.h"
#include "GUIPythonWindowXMLDialog.h"
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
  PyObject* WindowXMLDialog_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    WindowXMLDialog *self;

    self = (WindowXMLDialog*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    self->iWindowId = -1;
    PyObject* pyOXMLname, * pyOname;

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

      strXMLname = strSkinPath;
    }
    self->sFallBackPath  = strFallbackPath;
    self->sXMLFileName = strXMLname;
    self->bUsingXML = true;

    // create new GUIWindow
    if (!Window_CreateNewWindow((Window*)self, true))
    {
      // error is already set by Window_CreateNewWindow, just release the memory
      self->ob_type->tp_free((PyObject*)self);
      return NULL;
    }
    //((CGUIDialog*)(self->pWindow))->Load(strXMLname,false);
    ((CGUIWindow*)(self->pWindow))->SetCoordsRes(res);
    return (PyObject*)self;
  }
  PyDoc_STRVAR(windowXMLDialog__doc__,
    "WindowXMLDialog class.\n");

  PyMethodDef windowXMLDialog_methods[] = {
    {NULL, NULL, 0, NULL}
  };
// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

  PyTypeObject WindowXMLDialog_Type;

  void initWindowXMLDialog_Type()
  {
    PyInitializeTypeObject(&WindowXMLDialog_Type);

    WindowXMLDialog_Type.tp_name = "xbmcgui.WindowXMLDialog";
    WindowXMLDialog_Type.tp_basicsize = sizeof(WindowXMLDialog);
    WindowXMLDialog_Type.tp_dealloc = (destructor)Window_Dealloc;
    WindowXMLDialog_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    WindowXMLDialog_Type.tp_doc = windowXMLDialog__doc__;
    WindowXMLDialog_Type.tp_methods = windowXMLDialog_methods;
    WindowXMLDialog_Type.tp_base = &Window_Type;
    WindowXMLDialog_Type.tp_new = WindowXMLDialog_New;
  }
}

#ifdef __cplusplus
}
#endif

