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

#include "pyutil.h"
#include <wchar.h>
#include <vector>
#include "addons/Skin.h"
#include "tinyXML/tinyxml.h"
#include "utils/CharsetConverter.h"
#include "CriticalSection.h"
#include "SingleLock.h"

using namespace std;

static int iPyXBMCGUILockRef = 0;
static TiXmlDocument pySkinReferences;

#ifndef __GNUC__
#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")
#endif

namespace PYXBMC
{
  int PyXBMCGetUnicodeString(string& buf, PyObject* pObject, int pos)
  {
    // TODO: UTF-8: Does python use UTF-16?
    //              Do we need to convert from the string charset to UTF-8
    //              for non-unicode data?
    if (PyUnicode_Check(pObject))
    {
      // this will probably not really work since the python DLL assumes that
      // that wchar_t is 2 bytes and linux is actually 4 bytes. That's why
      // so building a CStdStringW will not work
      //
      CStdString utf8String;

      CStdStringW utf16String = (wchar_t*) PyUnicode_AsUnicode(pObject);
      g_charsetConverter.wToUTF8(utf16String, utf8String);

      buf = utf8String;
      return 1;
    }
    if (PyString_Check(pObject))
    {
      CStdString utf8String;
      g_charsetConverter.unknownToUTF8(PyString_AsString(pObject), utf8String);
      buf = utf8String;
      return 1;
    }

    // Object is not a unicode or a normal string.
    buf = "";
    if (pos != -1) PyErr_Format(PyExc_TypeError, "argument %.200i must be unicode or str", pos);
    return 0;
  }

  void PyXBMCGUILock()
  {
    if (iPyXBMCGUILockRef == 0) g_graphicsContext.Lock();
    iPyXBMCGUILockRef++;
  }

  void PyXBMCGUIUnlock()
  {
    if (iPyXBMCGUILockRef > 0)
    {
      iPyXBMCGUILockRef--;
      if (iPyXBMCGUILockRef == 0) g_graphicsContext.Unlock();
    }
  }

  static char defaultImage[1024];
  /*
   * Looks in references.xml for image name
   * If none exist return default image name
   */
  const char *PyXBMCGetDefaultImage(char* cControlType, char* cTextureType, char* cDefault)
  {
    // create an xml block so that we can resolve our defaults
    // <control type="type">
    //   <description />
    // </control>
    TiXmlElement control("control");
    control.SetAttribute("type", cControlType);
    TiXmlElement filler("description");
    control.InsertEndChild(filler);
    ADDON::g_SkinInfo.ResolveIncludes(&control, cControlType);

    // ok, now check for our texture type
    TiXmlElement *pTexture = control.FirstChildElement(cTextureType);
    if (pTexture)
    {
      // found our textureType
      TiXmlNode *pNode = pTexture->FirstChild();
      if (pNode && pNode->Value()[0] != '-')
      {
        strncpy(defaultImage, pNode->Value(), 1024);
        return defaultImage;
      }
    }
    return cDefault;
  }

  bool PyXBMCWindowIsNull(void* pWindow)
  {
    if (pWindow == NULL)
    {
      PyErr_SetString(PyExc_SystemError, "Error: Window is NULL, this is not possible :-)");
      return true;
    }
    return false;
  }

  void PyXBMCInitializeTypeObject(PyTypeObject* type_object)
  {
    static PyTypeObject py_type_object_header = { PyObject_HEAD_INIT(NULL) 0};
    int size = (long*)&(py_type_object_header.tp_name) - (long*)&py_type_object_header;

    memset(type_object, 0, sizeof(PyTypeObject));
    memcpy(type_object, &py_type_object_header, size);
  }
}


typedef std::pair<int(*)(void*), void*> Func;
typedef std::vector<Func> CallQueue;
CallQueue g_callQueue;
CCriticalSection g_critSectionPyCall;

void _PyXBMC_AddPendingCall(int(*func)(void*), void *arg)
{
  CSingleLock lock(g_critSectionPyCall);
  g_callQueue.push_back(Func(func, arg));
}

void _PyXBMC_MakePendingCalls()
{
  CSingleLock lock(g_critSectionPyCall);
  CallQueue::iterator iter = g_callQueue.begin();
  while (iter != g_callQueue.end())
  {
    int(*f)(void*) = (*iter).first;
    void* arg = (*iter).second;
    g_callQueue.erase(iter);
    lock.Leave();
    if (f)
      f(arg);
    //(*((*iter).first))((*iter).second);
    lock.Enter();
    iter = g_callQueue.begin();
  }
}

