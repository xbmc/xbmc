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

#include "stdafx.h"
#include "winxml.h"
#include "lib/libPython/Python/Include/Python.h"
#include "../XBPythonDll.h"
#include "pyutil.h"
#include "GUIPythonWindowXMLDialog.h"
#include "SkinInfo.h"
#include "Util.h"
#include "FileSystem/File.h"

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

using namespace std;

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
    PyObject* pyOXMLname = NULL;
    PyObject* pyOname = NULL;
    PyObject* pyDName = NULL;
    bool bForceDefaultSkin = false;

    string strXMLname, strFallbackPath;
    string strDefault = "Default";

    if (!PyArg_ParseTuple(args, (char*)"OO|Ob", &pyOXMLname, &pyOname, &pyDName, &bForceDefaultSkin )) return NULL;

    PyGetUnicodeString(strXMLname, pyOXMLname);
    PyGetUnicodeString(strFallbackPath, pyOname);
    if (pyDName) PyGetUnicodeString(strDefault, pyDName);

    RESOLUTION res;
    CStdString strSkinPath;
    if (!bForceDefaultSkin)
    {
      // Check to see if the XML file exists in current skin. If not use fallback path to find a skin for the script
      strSkinPath = g_SkinInfo.GetSkinPath(strXMLname, &res);
      
      if (!XFILE::CFile::Exists(strSkinPath))
      {
        // Check for the matching folder for the skin in the fallback skins folder
        CStdString basePath;
        CUtil::AddFileToFolder(strFallbackPath, "resources", basePath);
        CUtil::AddFileToFolder(basePath, "skins", basePath);
        CUtil::AddFileToFolder(basePath, CUtil::GetFileName(g_SkinInfo.GetBaseDir()), basePath);
        strSkinPath = g_SkinInfo.GetSkinPath(strXMLname, &res, basePath);
        if (!XFILE::CFile::Exists(strSkinPath))
        {
          // Finally fallback to the DefaultSkin as it didn't exist in either the XBMC Skin folder or the fallback skin folder
          bForceDefaultSkin = true;
        }
      }
    }

    if (bForceDefaultSkin)
    {
      CSkinInfo skinInfo;
      CStdString basePath;
      CUtil::AddFileToFolder(strFallbackPath, "resources", basePath);
      CUtil::AddFileToFolder(basePath, "skins", basePath);
      CUtil::AddFileToFolder(basePath, strDefault, basePath);

      skinInfo.Load(basePath);
      // if no skin.xml file exists default to PAL_4x3 and PAL_16x9
      if (skinInfo.GetDefaultResolution() == INVALID)
        skinInfo.SetDefaults();
      strSkinPath = skinInfo.GetSkinPath(strXMLname, &res, basePath);

      if (!XFILE::CFile::Exists(strSkinPath))
      {
        PyErr_SetString(PyExc_TypeError, "XML File for Window is missing");
        return NULL;
      }
    }

    self->sFallBackPath = strFallbackPath;
    self->sXMLFileName = strSkinPath;
    self->bUsingXML = true;

    // create new GUIWindow
    if (!Window_CreateNewWindow((Window*)self, true))
    {
      // error is already set by Window_CreateNewWindow, just release the memory
      self->sFallBackPath.~string();          
      self->sXMLFileName.~string();          
      self->ob_type->tp_free((PyObject*)self);
      return NULL;
    }
    ((CGUIWindow*)(self->pWindow))->SetCoordsRes(res);
    return (PyObject*)self;
  }

  PyDoc_STRVAR(windowXMLDialog__doc__,
    "WindowXMLDialog class.\n"
    "\n"
    "WindowXMLDialog(self, xmlFilename, scriptPath[, defaultSkin, forceFallback) -- Create a new WindowXMLDialog script.\n"
    "\n"
    "xmlFilename     : string - the name of the xml file to look for.\n"
    "scriptPath      : string - path to script. used to fallback to if the xml doesn't exist in the current skin. (eg os.getcwd())\n"
    "defaultSkin     : [opt] string - name of the folder in the skins path to look in for the xml. (default='Default')\n"
    "forceFallback   : [opt] boolean - if true then it will look only in the defaultSkin folder. (default=False)\n"
    "\n"
    "*Note, skin folder structure is eg(resources/skins/Default/PAL)\n"
    "\n"
    "example:\n"
    " - ui = GUI('script-Lyrics-main.xml', os.getcwd(), 'LCARS', True)\n"
    "   ui.doModal()\n"
    "   del ui\n");

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

    WindowXMLDialog_Type.tp_name = (char*)"xbmcgui.WindowXMLDialog";
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

