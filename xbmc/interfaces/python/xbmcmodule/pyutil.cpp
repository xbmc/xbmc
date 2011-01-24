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
#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"
#include "Application.h"

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

      // Python unicode objects are UCS2 or UCS4 depending on compilation
      // options, wchar_t is 16-bit or 32-bit depending on platform.
      // Avoid the complexity by just letting python convert the string.
      PyObject *utf8_pyString = PyUnicode_AsUTF8String(pObject);

      if (utf8_pyString)
      {
        buf = PyString_AsString(utf8_pyString);
        Py_DECREF(utf8_pyString);
        return 1;
      }
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

  void PyXBMCWaitForThreadMessage(int message, int param1, int param2)
  {
    Py_BEGIN_ALLOW_THREADS
    ThreadMessage tMsg = {message, param1, param2};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);
    Py_END_ALLOW_THREADS
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
    g_SkinInfo->ResolveIncludes(&control);

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


struct SPending
{
  int(*func)(void*);
  void*          args;
  PyThreadState* state;
};

typedef std::vector<SPending> CallQueue;
static CallQueue g_callQueue;
static CCriticalSection g_critSectionPyCall;

void _PyXBMC_AddPendingCall(PyThreadState* state, int(*func)(void*), void *arg)
{
  CSingleLock lock(g_critSectionPyCall);
  SPending p;
  p.func  = func;
  p.args  = arg;
  p.state = state;
  g_callQueue.push_back(p);
}

void _PyXBMC_ClearPendingCalls(PyThreadState* state)
{
  CSingleLock lock(g_critSectionPyCall);
  for(CallQueue::iterator it = g_callQueue.begin(); it!= g_callQueue.end();)
  {
    if(it->state == state)
      it = g_callQueue.erase(it);
    else
      it++;
  }
}

void _PyXBMC_MakePendingCalls()
{
  CSingleLock lock(g_critSectionPyCall);
  CallQueue::iterator iter = g_callQueue.begin();
  while (iter != g_callQueue.end())
  {
    SPending p(*iter);
    // only call when we are in the right thread state
    if(p.state != PyThreadState_Get())
    {
      iter++;
      continue;
    }
    g_callQueue.erase(iter);
    lock.Leave();
    if (p.func)
      p.func(p.args);
    //(*((*iter).first))((*iter).second);
    lock.Enter();
    iter = g_callQueue.begin();
  }
}

