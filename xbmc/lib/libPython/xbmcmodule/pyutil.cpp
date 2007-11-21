#include "stdafx.h"
#include "pyutil.h"
#include <wchar.h>
#include <vector>
#include "SkinInfo.h"

static int iPyGUILockRef = 0;
static TiXmlDocument pySkinReferences;

#ifndef __GNUC__
#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")
#endif

namespace PYXBMC
{
  int PyGetUnicodeString(string& buf, PyObject* pObject, int pos)
  {
    // TODO: UTF-8: Does python use UTF-16?
    //              Do we need to convert from the string charset to UTF-8
    //              for non-unicode data?
    if(PyUnicode_Check(pObject))
    {
      // this will probably not really work since the python DLL assumes that
      // that wchar_t is 2 bytes and linux is actually 4 bytes. That's why
      // so building a CStdStringW will not work
      CStdString utf8String;
      CStdStringW utf16String = (wchar_t*) PyUnicode_AsUnicode(pObject);
      //CStdStringW utf16String = (wchar_t*) PyUnicodeUCS4_AsUnicode(pObject);
      g_charsetConverter.wToUTF8(utf16String, utf8String);
      buf = utf8String;
      return 1;
    }
    if(PyString_Check(pObject))
    {
      CStdString utf8String;
      g_charsetConverter.stringCharsetToUtf8(PyString_AsString(pObject), utf8String);
      buf = utf8String;
      return 1;
    }
    // object is not an unicode ar normal string
    buf = "";
    if (pos != -1) PyErr_Format(PyExc_TypeError, "argument %.200i must be unicode or str", pos);
    return 0;
  }

  void PyGUILock()
  {
    if (iPyGUILockRef == 0) g_graphicsContext.Lock();
    iPyGUILockRef++;
  }

  void PyGUIUnlock()
  {
    if (iPyGUILockRef > 0)
    {
      iPyGUILockRef--;
      if (iPyGUILockRef == 0) g_graphicsContext.Unlock();
    }
  }

  static char defaultImage[1024];
  /*
   * Looks in references.xml for image name
   * If none exist return default image name
   */
  const char *PyGetDefaultImage(char* cControlType, char* cTextureType, char* cDefault)
  {
    // create an xml block so that we can resolve our defaults
    // <control type="type">
    //   <description />
    // </control>
    TiXmlElement control("control");
    control.SetAttribute("type", cControlType);
    TiXmlElement filler("description");
    control.InsertEndChild(filler);
    g_SkinInfo.ResolveIncludes(&control, cControlType);

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

  bool PyWindowIsNull(void* pWindow)
  {
    if (pWindow == NULL)
    {
      PyErr_SetString(PyExc_SystemError, "Error: Window is NULL, this is not possible :-)");
      return true;
    }
    return false;
  }

  void PyInitializeTypeObject(PyTypeObject* type_object)
  {
    static PyTypeObject py_type_object_header = { PyObject_HEAD_INIT(NULL) 0};
    int size = (long*)&(py_type_object_header.tp_name) - (long*)&py_type_object_header;

    memset(type_object, 0, sizeof(PyTypeObject));
    memcpy(type_object, &py_type_object_header, size);
  }
}

#ifdef _LINUX

typedef std::pair<int(*)(void*), void*> Func;
typedef std::vector<Func> CallQueue;
CallQueue g_callQueue;
CRITICAL_SECTION g_critSectionPyCall;

void PyInitPendingCalls()
{
  static bool first_call = true;
  if (first_call) 
  {
    InitializeCriticalSection(&g_critSectionPyCall);
    first_call = false;
  }
}

void _Py_AddPendingCall(int(*func)(void*), void *arg)
{
  PyInitPendingCalls();
  EnterCriticalSection(&g_critSectionPyCall);
  g_callQueue.push_back(Func(func, arg));
  LeaveCriticalSection(&g_critSectionPyCall);
}

void _Py_MakePendingCalls()
{
  PyInitPendingCalls();
  EnterCriticalSection(&g_critSectionPyCall);

  CallQueue::iterator iter = g_callQueue.begin();
  while (iter != g_callQueue.end())
  {
    int(*f)(void*) = (*iter).first;
    void* arg = (*iter).second;
    g_callQueue.erase(iter);
    LeaveCriticalSection(&g_critSectionPyCall);
    if (f)
      f(arg);
    //(*((*iter).first))((*iter).second);
    EnterCriticalSection(&g_critSectionPyCall);
    iter = g_callQueue.begin();
  }  
  LeaveCriticalSection(&g_critSectionPyCall);
}

#endif
