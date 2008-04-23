#ifndef _LINUX
#include "lib/libPython/python/Python.h"
#else
#include <python2.4/Python.h>
#endif
#include "window.h"
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{

  typedef struct {
    PyObject_HEAD_XBMC_WINDOW
  } WindowXML;

  typedef struct {
    PyObject_HEAD_XBMC_WINDOW
  } WindowXMLDialog;

  extern PyTypeObject WindowXML_Type;
  extern PyTypeObject WindowXMLDialog_Type;

  void initWindowXML_Type();
  void initWindowXMLDialog_Type();
}

#ifdef __cplusplus
}
#endif
