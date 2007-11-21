#include "stdafx.h"
#include "winxml.h"
#ifndef _LINUX
#include "../python/Python.h"
#else
#include <python2.4/Python.h>
#include "../XBPythonDll.h"
#endif
#include "pyutil.h"
#include "GUIPythonWindowXMLDialog.h"
#include "../../../Application.h"
#include "../../../../guilib/SkinInfo.h"
#include "../../../Util.h"

#define ACTIVE_WINDOW  m_gWindowManager.GetActiveWindow()

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
  PyObject* WindowXMLDialog_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    WindowXMLDialog *self;

    self = (WindowXMLDialog*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    new(&self->sXMLFileName) string();
    new(&self->sFallBackPath) string();
    new(&self->vecControls) std::vector<Control*>();    
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
        CUtil::AddFileToFolder(strFallbackPath, "skins", strSkinPath);
        CUtil::AddFileToFolder(strSkinPath, strDefault, strSkinPath);
        CUtil::AddFileToFolder(strSkinPath, "pal", strSkinPath);
        CUtil::AddFileToFolder(strSkinPath, strXMLname, strSkinPath);
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
    "WindowXMLDialog class.\n"
    "\n"
    "WindowXMLDialog(self, XMLname, fallbackPath[, defaultskinname, forceFallback) -- Create a new WindowXMLDialog to rendered a xml onto it.\n"
    "\n"
    "XMLname        : string - the name of the xml file to look for.\n"
    "fallbackPath   : string - the directory to fallback to if the xml doesn't exist in the current skin.\n"
    "defaultskinname: [opt] string - name of the folder in the fallback path to look in for the xml. 'Default' is used if this is not set.\n"
    "forceFallback  : [opt] boolean - if true then it will look only in the defaultskinname folder.\n"
    );

  PyMethodDef windowXMLDialog_methods[] = {
    {NULL, NULL, 0, NULL}
  };
// Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

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
    WindowXMLDialog_Type.tp_base = &WindowXML_Type;
    WindowXMLDialog_Type.tp_new = WindowXMLDialog_New;
  }
}

#ifdef __cplusplus
}
#endif

