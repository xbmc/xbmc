#include "../../../stdafx.h"
#include "winxml.h"
#include "..\python\python.h"
#include "pyutil.h"
#include "..\..\..\application.h"


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
    PyObject* pyOXMLname = 0;
    string strXMLname;
    if (!PyArg_ParseTuple(args, "O", &pyOXMLname, &self->iWindowId)) return NULL;
    PyGetUnicodeString( strXMLname, pyOXMLname);
    CLog::Log(LOGFATAL, "WindowXML Xml File: %s.", strXMLname.c_str());
    self->sXMLFileName = strXMLname;
    self->bUsingXML = true;
    // create new GUIWindow
    if (!Window_CreateNewWindow((Window*)self, true))
    {
      // error is already set by Window_CreateNewWindow, just release the memory
      self->ob_type->tp_free((PyObject*)self);
      return NULL;
    }

    return (PyObject*)self;
  }

  PyDoc_STRVAR(windowXML__doc__,
    "WindowXML class.\n");
  PyMethodDef WindowXML_methods[] = {
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

